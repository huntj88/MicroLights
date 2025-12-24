# lwjson
LWJSON_VERSION="v1.7.0"
LWJSON_COMMIT="278848a551674d73c9e3b1045fd32c4e455a6314"

# tinyusb
TINYUSB_VERSION="0.18.0"
TINYUSB_COMMIT="86ad6e56c1700e85f1c5678607a762cfe3aa2f47"

# tinyexpr
TINYEXPR_COMMIT="4a7456e"

# Unity
UNITY_VERSION="v2.6.1"
UNITY_COMMIT="cbcd08fa7de711053a3deec6339ee89cad5d2697"

mkdir -p libs &&
cd libs &&

rm -rf lwjson &&
echo "Downloading lwjson $LWJSON_VERSION ($LWJSON_COMMIT)..." &&
curl -L "https://github.com/MaJerle/lwjson/archive/$LWJSON_COMMIT.tar.gz" > lwjson.tar.gz &&
tar -xzf lwjson.tar.gz &&
rm lwjson.tar.gz &&
mv "lwjson-$LWJSON_COMMIT" lwjson &&

rm -rf tinyusb &&
echo "Downloading tinyusb $TINYUSB_VERSION ($TINYUSB_COMMIT)..." &&
curl -L "https://github.com/hathach/tinyusb/archive/$TINYUSB_COMMIT.tar.gz" > tinyusb.tar.gz &&
tar -xzf tinyusb.tar.gz &&
rm tinyusb.tar.gz &&
mv "tinyusb-$TINYUSB_COMMIT" tinyusb &&

python3 tinyusb/tools/get_deps.py stm32c0 &&

rm -rf tinyexpr &&
echo "Downloading tinyexpr ($TINYEXPR_COMMIT)..." &&
curl -L "https://github.com/codeplea/tinyexpr/archive/$TINYEXPR_COMMIT.tar.gz" > tinyexpr.tar.gz &&
tar -xzf tinyexpr.tar.gz &&
rm tinyexpr.tar.gz &&
mv "tinyexpr-$TINYEXPR_COMMIT"* tinyexpr &&

rm -rf Unity &&
echo "Downloading Unity $UNITY_VERSION ($UNITY_COMMIT)..." &&
curl -L "https://github.com/ThrowTheSwitch/Unity/archive/$UNITY_COMMIT.tar.gz" > Unity.tar.gz &&
tar -xzf Unity.tar.gz &&
rm Unity.tar.gz &&
mv "Unity-$UNITY_COMMIT" Unity

