# A Dockerfile to make development setup easier. Run with:
# docker run -it --rm -e OPENAI_API_KEY=$OPENAI_API_KEY $(docker build -q .)

FROM ubuntu

ARG LLVM_VERSION=18

RUN apt update \
    && DEBIAN_FRONTEND=noninteractive apt install -y tzdata \
    && apt install -y python3 python3-pip \
    && apt install -y locales \
    && apt install -y autoconf automake bear bison build-essential cmake flex gdb git libgdbm-dev m4 texinfo \
    && apt install -y curl lsb-release wget software-properties-common gnupg \
    && curl -sSf https://apt.llvm.org/llvm.sh | bash -s -- ${LLVM_VERSION} all \
    && apt clean \
    && rm -rf /var/lib/apt/lists/*

# UTF-8.
RUN locale-gen en_US.UTF-8
RUN update-locale LANG=en_US.UTF-8
ENV LANG=en_US.UTF-8
ENV LANGUAGE=en_US:en
ENV LC_ALL=en_US.UTF-8

# LLVM/Clang.
RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-${LLVM_VERSION} 100 
RUN update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-${LLVM_VERSION} 100 
RUN update-alternatives --install /usr/bin/clangd clangd /usr/bin/clangd-${LLVM_VERSION} 100 
RUN update-alternatives --install /usr/bin/lldb lldb /usr/bin/lldb-${LLVM_VERSION} 100 

# ChatDBG.
COPY . /root/ChatDBG
RUN pip install --break-system-packages -e /root/ChatDBG
RUN python3 -c 'import chatdbg; print(f"command script import {chatdbg.__path__[0]}/chatdbg_lldb.py")' >> ~/.lldbinit
RUN echo 'settingsi set target.disable-aslr false' >> ~/.lldbinit
RUN python3 -c 'import chatdbg; print(f"source {chatdbg.__path__[0]}/chatdbg_gdb.py")' >> ~/.gdbinit

# BugBench.
RUN cd /root/ChatDBG/samples/bugbench && make all

WORKDIR /root/ChatDBG
