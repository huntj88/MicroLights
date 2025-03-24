cd libs &&

rm -rf lwjson &&
curl -L https://github.com/MaJerle/lwjson/archive/refs/tags/v1.7.0.tar.gz > lwjson.tar.gz &&
tar -xzf lwjson.tar.gz &&
rm lwjson.tar.gz &&
mv lwjson-1.7.0 lwjson &&

rm -rf tinyusb &&
curl -L https://github.com/hathach/tinyusb/archive/refs/tags/0.18.0.tar.gz > tinyusb.tar.gz &&
tar -xzf tinyusb.tar.gz &&
rm tinyusb.tar.gz &&
mv tinyusb-0.18.0 tinyusb &&

python3 tinyusb-0.18.0/tools/get_deps.py stm32c0