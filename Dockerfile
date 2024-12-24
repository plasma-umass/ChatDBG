# A Dockerfile to make development setup easier. Run with:
# docker run -it --rm -e OPENAI_API_KEY=$OPENAI_API_KEY $(docker build -q .)

FROM ubuntu

ARG LLVM_VERSION=18

RUN apt update
RUN DEBIAN_FRONTEND=noninteractive apt install -y tzdata

# Set UTF-8 locale.
RUN apt install -y locales
RUN locale-gen en_US.UTF-8
RUN update-locale LANG=en_US.UTF-8
ENV LANG=en_US.UTF-8
ENV LANGUAGE=en_US:en
ENV LC_ALL=en_US.UTF-8

# Install gdb and general build tools.
RUN apt install -y build-essential cmake gdb git

# Install LLVM/Clang.
RUN apt install -y curl lsb-release wget software-properties-common gnupg
RUN curl -sSf https://apt.llvm.org/llvm.sh | bash -s -- ${LLVM_VERSION} all
RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-${LLVM_VERSION} 100 
RUN update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-${LLVM_VERSION} 100 
RUN update-alternatives --install /usr/bin/clangd clangd /usr/bin/clangd-${LLVM_VERSION} 100 
RUN update-alternatives --install /usr/bin/lldb lldb /usr/bin/lldb-${LLVM_VERSION} 100 

# Install Python and ChatDBG.
RUN apt install -y python3 python3-pip
COPY . /ChatDBG
RUN pip install --break-system-packages -e /ChatDBG
RUN python3 -c 'import chatdbg; print(f"command script import {chatdbg.__path__[0]}/chatdbg_lldb.py")' >> ~/.lldbinit
RUN echo 'settings set target.disable-aslr false' >> ~/.lldbinit
RUN python3 -c 'import chatdbg; print(f"source {chatdbg.__path__[0]}/chatdbg_gdb.py")' >> ~/.gdbinit

WORKDIR /ChatDBG
