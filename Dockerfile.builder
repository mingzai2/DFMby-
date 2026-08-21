FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV ANDROID_NDK_VERSION=r26d
ENV ANDROID_API=24

RUN apt-get update && apt-get install -y \
    wget \
    unzip \
    git \
    cmake \
    make \
    zip \
    && rm -rf /var/lib/apt/lists/*

RUN wget -q https://dl.google.com/android/repository/android-ndk-${ANDROID_NDK_VERSION}-linux.zip -O /tmp/ndk.zip \
    && unzip -q /tmp/ndk.zip -d /opt/ \
    && mv /opt/android-ndk-${ANDROID_NDK_VERSION} /opt/android-ndk \
    && rm /tmp/ndk.zip

ENV ANDROID_NDK_HOME=/opt/android-ndk
ENV PATH=${ANDROID_NDK_HOME}/toolchains/llvm/prebuilt/linux-x86_64/bin:$PATH

WORKDIR /build

COPY zygisk/ /build/zygisk/
COPY module.prop /build/
COPY customize.sh /build/
COPY post-fs-data.sh /build/
COPY service.sh /build/
COPY boot-completed.sh /build/
COPY zn_modules.txt /build/
COPY sepolicy.rule /build/
COPY README.md /build/
COPY build.sh /build/
COPY pack.sh /build/

RUN chmod +x /build/build.sh /build/pack.sh /build/customize.sh /build/post-fs-data.sh /build/service.sh /build/boot-completed.sh

RUN /build/build.sh && /build/pack.sh

CMD ["bash"]
