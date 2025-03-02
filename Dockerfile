FROM ubuntu:24.04
ADD . /workspace/dftracer
WORKDIR /workspace/dftracer
RUN apt-get update && apt-get install -y python3 python3-pip openmpi-bin openmpi-common libopenmpi-dev
RUN apt-get install -y git cmake python3.10 python3-pip python3-venv cmake
RUN python3 -m venv /workspace/venv
ENV PATH="/workspace/venv/bin:$PATH"
RUN cd /workspace/dftracer && ls && python3 -m pip install .