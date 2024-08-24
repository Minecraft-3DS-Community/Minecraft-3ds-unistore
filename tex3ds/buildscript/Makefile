#!/usr/bin/make -f

COMMON_PREFIX         := "$(CURDIR)/build"
COMMON_CONFIGURE      := ./configure CPPFLAGS="-I \"$(COMMON_PREFIX)/include\"" LDFLAGS="-L\"$(COMMON_PREFIX)/lib\"" --disable-shared --enable-static --prefix=$(COMMON_PREFIX)

libbzip2_PKG          := bzip2-1.0.6.tar.gz
libbzip2_PATH         := bzip2-1.0.6
libbzip2_CONFIGURE    := /bin/true
libbzip2_MAKE         := PREFIX="$(COMMON_PREFIX)"

libjpeg_PKG           := libjpeg-turbo-1.5.2.tar.gz
libjpeg_PATH          := libjpeg-turbo-1.5.2
libjpeg_CONFIGURE     := $(COMMON_CONFIGURE)

libtiff_PKG           := tiff-4.0.8.tar.gz
libtiff_PATH          := tiff-4.0.8
libtiff_CONFIGURE     := $(COMMON_CONFIGURE)

libpng_PKG            := libpng-1.6.34.tar.xz
libpng_PATH           := libpng-1.6.34
libpng_CONFIGURE      := $(COMMON_CONFIGURE)

zlib_PKG              := zlib-1.2.11.tar.xz
zlib_PATH             := zlib-1.2.11
zlib_CONFIGURE        := ./configure --static --prefix=$(COMMON_PREFIX)

ImageMagick_PKG       := ImageMagick.tar.gz
ImageMagick_PATH      := ImageMagick-7.0.7-4
ImageMagick_CONFIGURE := $(COMMON_CONFIGURE) --disable-openmp --with-gslib=no --without-fontconfig --without-freetype --without-lcms --without-openexr --without-pango --without-webp --without-x --without-xml

export LD_LIBRARY_PATH := "$(COMMON_PREFIX)/lib"
export PKG_CONFIG_PATH := "$(LD_LIBRARY_PATH)/pkgconfig"

.PHONY: all build clean

all:
	@echo "Use 'make tex3ds' to build static version of tex3ds"

tex3ds: ImageMagick
	@cd .. && \
		./autogen.sh && \
		./configure PKG_CONFIG_PATH="$(PKG_CONFIG_PATH)" PKG_CONFIG="pkg-config --static" --prefix=$(DEVKITARM) && \
		$(MAKE) LDFLAGS="-static -pthread" && \
		strip $@

clean:
	@$(RM) -r build/ \
		$(libbzip2_PATH) \
		$(libjpeg_PATH) \
		$(libpng_PATH) \
		$(libtiff_PATH) \
		$(zlib_PATH) \
		$(ImageMagick_PATH)

build:
	@[ -d $@ ] || mkdir $@

libpng: zlib
ImageMagick: libbzip2 libjpeg libpng libtiff zlib

libbzip2 libjpeg libpng libtiff zlib ImageMagick: %: | build
	@echo "Building $@"
	@[ -d $($@_PKG) ] || tar -xaf $($@_PKG)
	@cd $($@_PATH) && \
		([ -f ./.configured ] || ($($@_CONFIGURE) && touch ./.configured)) && \
		([ -f ./.made ] || ($(MAKE) $($@_MAKE) && touch ./.made)) && \
		([ -f ./.installed ] || ($(MAKE) $($@_MAKE) install && touch ./.installed))
