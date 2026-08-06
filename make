rm -f pitch
g++ -I. pitch.cpp base58.cpp bech32.cpp bip39.cpp derive.cpp mnemonic.cpp crypto/*.cpp -o pitch $(pkg-config --cflags --libs sdl2) -lm -O2 -lssl -lcrypto -lsecp256k1 -Wno-deprecated-declarations
