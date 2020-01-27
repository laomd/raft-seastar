from fedora:31

COPY ./user.sh ./user.sh
RUN ./user.sh
RUN sudo dnf install -y git
RUN git clone https://github.com/laomd/raft-seastar.git
WORKDIR raft-seastar
RUN git submodule update --init
RUN sudo ./seastar/install-dependencies.sh
RUN sudo dnf install -y vim wget curl clang clang-tools-extra