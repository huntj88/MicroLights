python3 tinyusb/tools/get_deps.py stm32c0 &&

cd libs &&
rm -rf lwjson &&
curl -L https://github.com/MaJerle/lwjson/archive/refs/tags/v1.7.0.tar.gz > lwjson.tar.gz &&
tar -xzf lwjson.tar.gz &&
rm lwjson.tar.gz
