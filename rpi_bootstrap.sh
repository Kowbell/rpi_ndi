# copy me into ~/

sudo apt-get install -y build-essential git libavahi-client-dev

mkdir dev
cd dev

git clone https://github.com/Kowbell/rpi_ndi.git
cd rpi_ndi



tar -xzf ndi_sdk.tar.gz

sudo cp ndi_sdk/lib/aarch64-rpi4-linux-gnueabi/libndi.so /usr/lib/
sudo cp ndi_sdk/lib/aarch64-rpi4-linux-gnueabi/libndi.so.5 /usr/lib/
sudo cp ndi_sdk/lib/aarch64-rpi4-linux-gnueabi/libndi.so.5.6.0 /usr/lib/



chmod +x ./compile.sh



echo "Make sure /boot/cmdline.txt has 'Video=Composite-1:720x480M-32@60'"
echo "and /boot/config.txt has 'dtoverlay=vc4-fkms-v3d,composite', 'sdtv_mode=0', 'sdtv_aspect=1', and 'enable_tvout=1'"

# for copy and pasting, that last one was:
# dtoverlay=vc4-fkms-v3d,composite
# sdtv_mode=0
# sdtv_aspect=1
# enable_tvout=1