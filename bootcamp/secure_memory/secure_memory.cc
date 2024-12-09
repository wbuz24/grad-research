
#include <algorithm>
#include "bootcamp/secure_memory/secure_memory.hh"
#include "debug/SecureMemory.hh"


namespace gem5{
SecureMemory::SecureMemory(const SecureMemoryParams& params):
    ClockedObject(params),
    cpuSidePort(this, name() + ".cpu_side_port"),
    memSidePort(this, name() + ".mem_side_port"),
    bufferEntries(params.inspection_buffer_entries),
    buffer(clockPeriod()),
    responseBufferEntries(params.response_buffer_entries),
    responseBuffer(clockPeriod()),
    newRangeList(),
    integrity_levels(),
    data_level(0), // set after object construction in setup()
    counter_level(0), // set after object construction in setup()
    pending_tree_authentication(),
    pending_hmac(),
    pending_untrusted_packets(),
    nextReqSendEvent([this](){ processNextReqSendEvent(); }, name() + ".nextReqSendEvent"),
    nextReqRetryEvent([this](){ processNextReqRetryEvent(); }, name() + ".nextReqRetryEvent"),
    nextRespSendEvent([this](){ processNextRespSendEvent(); }, name() + ".nextRespSendEvent"),
    nextRespRetryEvent([this](){ processNextRespRetryEvent(); }, name() + ".nextRespRetryEvent"),
    stats(SecureMemoryStats(this))
{}

void
SecureMemory::init()
{
    cpuSidePort.sendRangeChange();

    // setup address range for secure memory metadata
    AddrRangeList ranges = memSidePort.getAddrRanges();
    assert(ranges.size() == 1);

    uint64_t start = ranges.front().start();
    uint64_t end = ranges.front().end() - (ranges.front().size()/2);

    DPRINTF(SecureMemory,"Setting the new range to have start=%x, and end=%x.\n", start, end);
    newRangeList.push_front(AddrRange(start,end));

    uint64_t hmac_bytes = ((end - start) / BLOCK_SIZE) * HMAC_SIZE;
    uint64_t counter_bytes = ((end - start) / PAGE_SIZE) * BLOCK_SIZE;

    // initialize integrity_levels
    uint64_t tree_offset = end + hmac_bytes;

    integrity_levels.push_front(start); // where does data start?
    integrity_levels.push_front(tree_offset); // where does tree start?

    uint64_t bytes_on_level = counter_bytes;
    do {
        integrity_levels.push_front(tree_offset + bytes_on_level); // level starting address
        tree_offset += bytes_on_level;
        bytes_on_level /= ARITY;
    } while (bytes_on_level > 1);

    integrity_levels.push_front(end); // hmac start
    integrity_levels.shrink_to_fit();

    data_level = integrity_levels.size() - 1;
    counter_level = data_level - 1;
}

Port&
SecureMemory::getPort(const std::string &if_name, PortID idx)
{
    if (if_name == "cpu_side_port") {
        return cpuSidePort;
    } else if (if_name == "mem_side_port") {
        return memSidePort;
    } else {
        return ClockedObject::getPort(if_name, idx);
    }
}

AddrRangeList SecureMemory::CPUSidePort::getAddrRanges() const
{
     return owner->getAddrRanges();
}

Tick SecureMemory::CPUSidePort::recvAtomic(PacketPtr pkt)
{
    DPRINTF(SecureMemory, "%s: Received pkt: %s in atomic mode.\n", __func__, pkt->print());
    return owner->recvAtomic(pkt);
}

void
SecureMemory::CPUSidePort::recvFunctional(PacketPtr pkt)
{
    DPRINTF(SecureMemory, "%s: Received pkt: %s in functional mode.\n", __func__, pkt->print());
    owner->recvFunctional(pkt);
}

bool
SecureMemory::CPUSidePort::recvTimingReq(PacketPtr pkt)
{
    DPRINTF(SecureMemory, "%s: Received pkt: %s in timing mode.\n", __func__, pkt->print());

    if (owner->recvTimingReq(pkt)) {
        needToSendRetry = false;
        return true;
    }
    needToSendRetry = true;
    return false;
}

AddrRangeList
SecureMemory::getAddrRanges() const
{
    AddrRangeList total = memSidePort.getAddrRanges();
    assert(total.size() == 1);

    uint64_t start = total.front().start();
    uint64_t end = total.front().end();
    fatal_if(!(start==0), "Start address must be zero. start=%x\n",start);
    uint64_t data_end =  end - (total.front().size()/2);
    AddrRange newRange(start,data_end);
    DPRINTF(SecureMemory, "Sending back AddrRange start %x, end%x\n", start, data_end);
    AddrRangeList newList({newRange});
    return newList;
   // return newRangeList;
}

void
SecureMemory::recvFunctional(PacketPtr pkt)
{
    memSidePort.sendFunctional(pkt);
}

Tick
SecureMemory::recvAtomic(PacketPtr pkt)
{
    return clockPeriod() + memSidePort.sendAtomic(pkt);
}

bool
SecureMemory::recvTimingReq(PacketPtr pkt)
{
    DPRINTF(SecureMemory, "%s: buffer size: %s, integrity_levels size: %s .\n", __func__, buffer.size(), integrity_levels.size());
    if (cpuSidePort.blocked() || ((buffer.size() + integrity_levels.size() * 2) >= bufferEntries)){
        scheduleReqRetryEvent(nextCycle());
        return false;
    }

    return handleRequest(pkt);


}

void
SecureMemory::MemSidePort::sendPacket(PacketPtr pkt)
{
    panic_if(blocked(), "Should never try to send if blocked!");
    panic_if(pkt == nullptr, "Invalid Packet");

    DPRINTF(SecureMemory, "%s: Sending pkt: %s.\n", __func__, pkt->print());
    if (!sendTimingReq(pkt)) {
        DPRINTF(SecureMemory, "%s: Failed to send pkt: %s.\n", __func__, pkt->print());
        blockedPacket = pkt;
    }
}

void
SecureMemory::processNextReqSendEvent()
{
    panic_if(memSidePort.blocked(), "Should never try to send if blocked!");
    panic_if(!buffer.hasReady(curTick()), "Should never try to send if no ready packets!");

    DPRINTF(SecureMemory,"In processNextReqSendEvent, buffer size:%d\n", buffer.size());
    stats.numRequestsFwded++;
    stats.totalbufferLatency += curTick() - buffer.frontTime();

    PacketPtr pkt = buffer.front();
    DPRINTF(SecureMemory,"Sending packet to memSidePort with address %x", pkt->getAddr());
    memSidePort.sendPacket(pkt);
    buffer.pop();
    scheduleReqRetryEvent(nextCycle());
    scheduleNextReqSendEvent(nextCycle());
}
void
SecureMemory::processNextReqRetryEvent()
{
    panic_if(!cpuSidePort.needRetry(), "Should never try to send retry if not needed!");
    if(!memSidePort.blocked()){
        cpuSidePort.sendRetryReq();
    }
    scheduleReqRetryEvent(nextCycle());
}

void
SecureMemory::scheduleReqRetryEvent(Tick when)
{
    if (cpuSidePort.needRetry() && !nextReqRetryEvent.scheduled()) {
        schedule(nextReqRetryEvent, std::max(curTick(),when));
    }
}

void
SecureMemory::scheduleNextReqSendEvent(Tick when)
{
    bool port_avail = !memSidePort.blocked();
    bool have_items = !buffer.empty();

    DPRINTF(SecureMemory,"In Schedule next request send event.\n");
    if (port_avail && have_items && !nextReqSendEvent.scheduled()) {
        Tick schedule_time = std::max(curTick(),buffer.firstReadyTime());
        schedule(nextReqSendEvent, schedule_time);
    }
    else{
        if(curTick() >= 4423238){
            DPRINTF(SecureMemory,"Failed to schedule the next request send event. Size of the buffer: %d\n", buffer.size());
        }
    }
}
void
SecureMemory::MemSidePort::recvReqRetry()
{
    panic_if(!blocked(), "Should never receive retry if not blocked!");

    DPRINTF(SecureMemory, "%s: Received retry signal.\n", __func__);

    panic_if(blockedPacket == nullptr, "Request blocked packet is null.");
    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;
    sendPacket(pkt);

    owner->recvReqRetry();

}
void
SecureMemory::recvReqRetry()
{
    scheduleNextReqSendEvent(nextCycle());
}



// Response Path Functions:
bool
SecureMemory::MemSidePort::recvTimingResp(PacketPtr pkt)
{
    DPRINTF(SecureMemory, "%s: Buffer size: %d, Received resp pkt: %s in timing mode.\n", __func__, owner->buffer.size(),pkt->print());
    if (owner->recvTimingResp(pkt)) {
        needToSendRetry = false;
        return true;
    }

    DPRINTF(SecureMemory, "%s: Buffer size: %d, Received resp pkt: %s in timing mode.\n", __func__, owner->buffer.size(),pkt->print());
    needToSendRetry = true;
    return false;
}


bool
SecureMemory::recvTimingResp(PacketPtr pkt)
{
    DPRINTF(SecureMemory, "In recvTimingResp function. ResponseBuffer size: %d\n",responseBuffer.size());
    if (responseBuffer.size() >= responseBufferEntries) {
        DPRINTF(SecureMemory, "Too many response buffer entries! \n");
        return false;
    }

    if(pkt->getAddr() < integrity_levels[hmac_level]){
        panic_if(!pkt->isResponse(), "Data packet response should be of RespRead type.");
    }
    return handleResponse(pkt);
    //responseBuffer.push(pkt, curTick());
  //  scheduleNextRespSendEvent(nextCycle());
//    return true;
}

void
SecureMemory::CPUSidePort::sendPacket(PacketPtr pkt)
{
    panic_if(blocked(), "Should never try to send if blocked!");

    DPRINTF(SecureMemory, "%s: Sending pkt: %s.\n", __func__, pkt->print());
    if (!sendTimingResp(pkt)) {
        DPRINTF(SecureMemory, "%s: Failed to send pkt: %s.\n", __func__, pkt->print());
        blockedPacket = pkt;
    }
}

void
SecureMemory::processNextRespSendEvent()
{
    panic_if(cpuSidePort.blocked(), "Should never try to send if blocked!");
    panic_if(!responseBuffer.hasReady(curTick()), "Should never try to send if no ready packets!");

    stats.numResponsesFwded++;
    stats.totalResponseBufferLatency += curTick() - responseBuffer.frontTime();

    PacketPtr pkt = responseBuffer.front();
    cpuSidePort.sendPacket(pkt);
    responseBuffer.pop();

    scheduleNextRespRetryEvent(nextCycle());
    scheduleNextRespSendEvent(nextCycle());
}
void
SecureMemory::processNextRespRetryEvent()
{
    panic_if(!memSidePort.needRetry(), "Should never try to send retry if not needed!");
    memSidePort.sendRetryResp();
}

void
SecureMemory::scheduleNextRespRetryEvent(Tick when)
{
    if (memSidePort.needRetry() && !nextRespRetryEvent.scheduled()) {
        schedule(nextRespRetryEvent, when);
    }
}

void
SecureMemory::scheduleNextRespSendEvent(Tick when)
{
    bool port_avail = !cpuSidePort.blocked();
    bool have_items = !responseBuffer.empty();

    if (port_avail && have_items && !nextRespSendEvent.scheduled()) {
        Tick schedule_time = std::max(curTick(),responseBuffer.firstReadyTime());
        schedule(nextRespSendEvent, schedule_time);
    }
}
void
SecureMemory::CPUSidePort::recvRespRetry()
{
    panic_if(!blocked(), "Should never receive retry if not blocked!");

    DPRINTF(SecureMemory, "%s: Received retry signal.\n", __func__);
    panic_if(blockedPacket == nullptr, "Response blocked packet is null.");
    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;
    sendPacket(pkt);

    if (!blocked()) {
        owner->recvRespRetry();
    }
}
void
SecureMemory::recvRespRetry()
{
    scheduleNextRespSendEvent(nextCycle());
}


SecureMemory::SecureMemoryStats::SecureMemoryStats(SecureMemory* secure_memory):
    statistics::Group(secure_memory),
    ADD_STAT(totalbufferLatency, statistics::units::Tick::get(), "Total inspection buffer latency."),
    ADD_STAT(numRequestsFwded, statistics::units::Count::get(), "Number of requests forwarded."),
    ADD_STAT(totalResponseBufferLatency, statistics::units::Tick::get(), "Total response buffer latency."),
    ADD_STAT(numResponsesFwded, statistics::units::Count::get(), "Number of responses forwarded.")
{}


uint64_t
SecureMemory::getHmacAddr(uint64_t child_addr)
{
    // AddrRangeList ranges = memSidePort.getAddrRanges();
    // assert(ranges.size() == 1);

    uint64_t start = newRangeList.front().start();
    uint64_t end = newRangeList.front().end();

    if (!(child_addr >= start && child_addr < end)) {
        // this is a check for something that isn't metadata
        return (uint64_t) -1;
    }

    // raw location, not word aligned
    uint64_t hmac_addr = integrity_levels[hmac_level] + ((child_addr / BLOCK_SIZE) * HMAC_SIZE);

    // word aligned
    return hmac_addr - (hmac_addr % BLOCK_SIZE);
}

uint64_t
SecureMemory::getParentAddr(uint64_t child_addr)
{
     AddrRangeList ranges = memSidePort.getAddrRanges();
    // assert(ranges.size() == 1);

    // uint64_t start = ranges.front().start();
     uint64_t end = (ranges.front().start() + ranges.front().size() / 2 );

    uint64_t start = newRangeList.front().start();
    //uint64_t end = newRangeList.front().end();

    //DPRINTF(SecureMemory, "In getParentAddr, start addr %x, end addr %x, child_addr %x\n",start,end, child_addr);
    if (child_addr >= start && child_addr < end) {
        // child is data, get the counter
        return integrity_levels[counter_level] + ((child_addr / PAGE_SIZE) * BLOCK_SIZE);
    }

    for (int i = counter_level; i > root_level; i--) {
        if (child_addr >= integrity_levels[i] && child_addr < integrity_levels[i - 1]) {
            // we belong to this level
            uint64_t index_in_level = (child_addr - integrity_levels[i]) / BLOCK_SIZE;
            return integrity_levels[i - 1] + ((index_in_level / ARITY) * BLOCK_SIZE);
        }
    }

    assert(child_addr == integrity_levels[root_level]);
    // assert(false); // we shouldn't ever get here
    return (uint64_t) -1;
}

void
SecureMemory::verifyChildren(PacketPtr parent)
{
    if (parent->getAddr() < integrity_levels[hmac_level]) { //is addr in the data range?
        bool awaiting_hmac = false;
        for (uint64_t addr: pending_hmac) {
            if (addr == parent->getAddr()) { //this pkt is data, lets check if we are waiting for the hmac
                DPRINTF(SecureMemory, "Marking pkt with addr %x as waiting for hmac\n",addr);
                awaiting_hmac = true;
                break;
            }
        }

        if (!awaiting_hmac) {
            //this data packet is no longer waiting for the hmac
            // we are authenticated!
            pending_tree_authentication.erase(parent->getAddr());

            if (parent->isWrite()) {
                // also send writes for all of the metadata
                //memSidePort.sendPacket(parent);
                fatal_if(buffer.size() > bufferEntries,"Buffer size will exceed number of entries");
                buffer.push(parent, curTick());
                DPRINTF(SecureMemory, "Write request needs to be sent back to memory for addr %x\n",parent->getAddr());
                scheduleNextReqSendEvent(nextCycle());

            } else {
                //cpuSidePort.sendPacket(parent)
                //send back the data to the CPU. it has been authenticated
                fatal_if(responseBuffer.size() > responseBufferEntries,"Response buffer size will exceed number of entries");
                fatal_if(parent->getAddr() > integrity_levels[hmac_level],"Response packet is not for data.");
                responseBuffer.push(parent, curTick());
                DPRINTF(SecureMemory, "Data request for addr %x is authenticated and decrypted. Sending back to cpu.\n",parent->getAddr());
                panic_if(!parent->isResponse(), "Data packet response is not a ReadResp request!");
                scheduleNextRespSendEvent(nextCycle());
            }
        }

        //data packet still waiting for hmac to come back. nothing else we can do
        return;
    }

    //we end here if the parent is for a metadata block (not data)
    std::vector<PacketPtr> to_call_verify;

    // verify all packets that have returned and are waiting
    for (auto it = pending_untrusted_packets.begin();
              it != pending_untrusted_packets.end(); ) {
        if (getParentAddr((*it)->getAddr()) == parent->getAddr()) {
            // Found a packet waiting for this address to verify, since it came back from memory adding it to the call to verify list
            panic_if(!((*it)->isResponse()), "Data packet response is not a ReadResp request!");
            to_call_verify.push_back(*it);
            it = pending_untrusted_packets.erase(it);
        } else {
            ++it;
        }
    }

    // all done, free/remove node
    DPRINTF(SecureMemory, "Removing packet with address %x.\n",parent->getAddr());
    delete parent;

    for (PacketPtr pkt: to_call_verify) {
       // DPRINTF(SecureMemory, "Calling verifyChildren for addr %x.\n",pkt->getAddr());
        panic_if(!pkt->isResponse(), "Data packet response is not a ReadResp request!");
        verifyChildren(pkt);
    }
}

bool
SecureMemory::handleResponse(PacketPtr pkt)
{
    if (pkt->isWrite() && pkt->getAddr() < integrity_levels[hmac_level]) {
        //data was written to memory, send back response.
        // cpuSidePort.sendPacket(pkt);
        fatal_if(responseBuffer.size() > responseBufferEntries,"Response buffer size will exceed number of entries");
        responseBuffer.push(pkt, curTick());
        DPRINTF(SecureMemory, "Sending back response for write request for addr %x\n",pkt->getAddr());
        scheduleNextRespSendEvent(nextCycle());
        return true;
    }

    if (pkt->getAddr() >= integrity_levels[hmac_level] && pkt->getAddr() < integrity_levels[counter_level]) {
        //Received hmac from memory, authenticate the data waiting for it
        bool foundData = false;
        for (auto it = pending_hmac.begin();
                  it != pending_hmac.end(); ) {
            if (getHmacAddr(*it) == pkt->getAddr()) {
                uint64_t temp = (*it);
                DPRINTF(SecureMemory, "Removing address %x from the pending_hmac list for hmac %x\n", temp, pkt->getAddr());
                it = pending_hmac.erase(it);
                foundData = true;

                //TODO: need to verify that if the hmac arrives last we still call verify children for the waiting data blocks
                // using simple memory, so we can assume hmac
                // /NOT TRUE! will always be verified first and not worry
                // about the case where cipher happens before verification
                 if (pending_tree_authentication.find(getParentAddr(temp)) == pending_tree_authentication.end()){
                    panic_if(temp > integrity_levels[hmac_level],"Address not for data\n");
                    for (auto it = pending_untrusted_packets.begin();
                                it != pending_untrusted_packets.end(); ) {
                                    if(temp == (*it)->getAddr()){
                                        DPRINTF(SecureMemory, "hmac received after tree verification, sending back response for the data block %x\n",temp);
                                        panic_if(!((*it)->isResponse()), "Data packet response is not a ReadResp request!");
                                        verifyChildren(*it);
                                        break;
                                    }
                                    it++;
                                }
                 }
            } else {
                ++it;
            }
        }
        //panic_if(foundData == false,"Could not find data block waiting for this hmac addr=%x.\n", pkt->getAddr());
        DPRINTF(SecureMemory, "hmac returned from memory, removing hmac pkt with address %x\n",pkt->getAddr());
        delete pkt;
        return true;
    }

    // address received is for an integrity tree node or counter block or data block
    pending_tree_authentication.erase(pkt->getAddr());
    if (pkt->getAddr() == integrity_levels[root_level]) {
        // value is at the top of the tree thus can be verified against the root, authenticate children
        DPRINTF(SecureMemory, "Received response fom memory for root for addr %x, calling verifyChildren.\n",pkt->getAddr());
        verifyChildren(pkt);
    } else {
        // move from pending address to pending metadata stored
        // in on-chip buffer for authentication
        DPRINTF(SecureMemory, "Received metadata or data block from memory for addr %x\n",pkt->getAddr());
        panic_if(!(pkt->isResponse()), "Data packet response is not a ReadResp request!");
        pending_untrusted_packets.insert(pkt);
    }

    return true;
}

bool
SecureMemory::handleRequest(PacketPtr pkt)
{
    std::vector<uint64_t> metadata_addrs;


    uint64_t child_addr = pkt->getAddr();

    uint64_t hmac_addr = getHmacAddr(child_addr);

    metadata_addrs.push_back(hmac_addr);
   DPRINTF(SecureMemory, "Pushed back into metadata_addr hmac addr: %x\n", hmac_addr);
    do {
        metadata_addrs.push_back(getParentAddr(child_addr));
        child_addr = metadata_addrs.back();
        DPRINTF(SecureMemory, "Pushed back into metadata_addr child addr: %x\n", child_addr);
    } while (child_addr != integrity_levels[root_level]);

    pending_tree_authentication.insert(pkt->getAddr());

    DPRINTF(SecureMemory, "Pushing address: %x into the pending_hmac list for hmac %x\n", pkt->getAddr(),hmac_addr);
    pending_hmac.insert(pkt->getAddr());

    if (pkt->isWrite() && pkt->hasData()) {
        panic_if(!(pkt->isResponse()), "Data packet response is not a ReadResp request!");
        pending_untrusted_packets.insert(pkt);
    } else if (pkt->isRead()) {
        //memSidePort.sendPacket(pkt);
        fatal_if(buffer.size() > bufferEntries,"Buffer size will exceed number of entries");
        buffer.push(pkt, curTick());
        DPRINTF(SecureMemory, "%s: pushing packet pkt: %s .\n", __func__, pkt->print());
    }

    int i =0; //iterator to count the number of metadta nodes added to the buffer
    for (uint64_t addr: metadata_addrs) {
        RequestPtr req = std::make_shared<Request>(addr, BLOCK_SIZE, 0, 0);
        PacketPtr metadata_pkt = Packet::createRead(req);
        i++;
        metadata_pkt->allocate();

        if (addr != hmac_addr) {
            // note: we can't save the packet itself because it may be deleted
            // by the memory device :-)
            pending_tree_authentication.insert(addr);
        }

        //memSidePort.sendPacket(metadata_pkt);
        fatal_if(buffer.size() > bufferEntries,"Buffer size will exceed number of entries");
        buffer.push(metadata_pkt, curTick()+i);
        DPRINTF(SecureMemory, "%s: pushing packet metadata pkt: %s .\n", __func__, metadata_pkt->print());
    }

    scheduleNextReqSendEvent(nextCycle());
    return true;
}

};
