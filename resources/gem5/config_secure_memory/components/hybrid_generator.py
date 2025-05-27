# Copyright (c) 2021 The Regents of the University of California
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

from typing import List, Union

from math import log

from gem5.utils.override import overrides
from gem5.components.processors.abstract_generator import (
    AbstractGenerator,
    partition_range,
)
from gem5.components.processors.linear_generator_core import (
    LinearGeneratorCore,
)
from gem5.components.processors.random_generator_core import (
    RandomGeneratorCore,
)


class HybridGenerator(AbstractGenerator):
    def __init__(
        self,
        num_cores: int = 2,
        duration: str = "1ms",
        rate: str = "1GB/s",
        block_size: int = 8,
        min_addr: int = 0,
        max_addr: int = 131072,
        rd_perc: int = 100,
        data_limit: int = 0,
    ) -> None:
        if num_cores < 2:
            raise ValueError("num_cores should be >= 2!")
        super().__init__(
            cores=self._create_cores(
                num_cores=num_cores,
                duration=duration,
                rate=rate,
                block_size=block_size,
                min_addr=min_addr,
                max_addr=max_addr,
                rd_perc=rd_perc,
                data_limit=data_limit,
            )
        )
        """The hybrid generator

        This class defines an external interface to create a list of linear and
        random generator cores that could replace the processing cores in a board.

        :param num_cores: The number of linear generator cores to create.
        :param duration: The number of ticks for the generator to generate
                         traffic.
        :param rate: The rate at which the synthetic data is read/written.
        :param block_size: The number of bytes to be read/written with each
                           request.
        :param min_addr: The lower bound of the address range the generator
                         will read/write from/to.
        :param max_addr: The upper bound of the address range the generator
                         will read/write from/to.
        :param rd_perc: The percentage of read requests among all the generated
                        requests. The write percentage would be equal to
                        ``100 - rd_perc``.
        :param data_limit: The amount of data in bytes to read/write by the
                           generator before stopping generation.
        """

    def _create_cores(
        self,
        num_cores: int,
        duration: str,
        rate: str,
        block_size: int,
        min_addr: int,
        max_addr: int,
        rd_perc: int,
        data_limit: int,
    ) -> List[Union[LinearGeneratorCore, RandomGeneratorCore]]:
        """
        The helper function to create the cores for the generator, it will use
        the same inputs as the constructor function.
        """

        def get_num_linear_cores(num_cores: int):
            """
            Returns the largest power of two that is smaller than num_cores
            """
            if num_cores & (num_cores - 1) == 0:
                return num_cores // 2
            else:
                return 2 ** int(log(num_cores, 2))

        """
        Initializing variables
        """
        # (1)
        core_list = []

        num_linear_cores = get_num_linear_cores(num_cores)
        num_random_cores = num_cores - num_linear_cores

        """
        partition_range(): Partitions the range of min_addr to max_addr into
        num_linear_cores sections

        Each section is represented as a tuple <min, max>, where min
        is the smallest address for that section and max is the largest
        address for that section
        """
        # (2)
        addr_ranges = partition_range(min_addr, max_addr, num_linear_cores)

        """
        The first n cores (where n is the largest power of 2 that is
        less than num_cores) will be Linear Generator Cores

        Each Linear Generator Core gets a section of memory, and all
        Linear Generator Cores cover the range of min_addr to max_addr
        """
        # (3)
        for i in range(num_linear_cores):
            core_list.append(
                LinearGeneratorCore(
                    duration=duration,
                    rate=rate,
                    block_size=block_size,
                    min_addr=addr_ranges[i][0],
                    max_addr=addr_ranges[i][1],
                    rd_perc=rd_perc,
                    data_limit=data_limit,
                )
            )

        """
        The remaining cores will be Random Generator Cores

        Each Random Generator Core covers the range of min_addr to max_addr
        """
        # (4)
        for i in range(num_random_cores):
            core_list.append(
                RandomGeneratorCore(
                    duration=duration,
                    rate=rate,
                    block_size=block_size,
                    min_addr=min_addr,
                    max_addr=max_addr,
                    rd_perc=rd_perc,
                    data_limit=data_limit,
                )
            )

        """
        Return our list of cores
        """
        # (5)
        return core_list

    @overrides(AbstractGenerator)
    def start_traffic(self) -> None:
        for core in self.cores:
            core.start_traffic()
