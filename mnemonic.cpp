// Copyright (c) 2026 barrystyle

#include <base58.h>
#include <bip39.h>
#include <mnemonic.h>

#include <openssl/sha.h>

#include <string>
#include <vector>

void entropy_to_mnemonic(char* entropy_in, int bitlen, std::string& mnemonic)
{
    std::vector<uint8_t> entropy_vec;
    for (int i=0; i < bitlen / 8; i++) {
        entropy_vec.push_back(entropy_in[i]);
    }
    mnemonic = CMnemonic::FromData(entropy_vec, bitlen / 8);
}

void mnemonic_to_seed(std::string& phrase, std::string passphrase, std::vector<uint8_t>& seed_out)
{
    CMnemonic::ToSeed(phrase, passphrase, seed_out);
}

void calculate_from_seed(std::vector<uint8_t>& seed)
{
    pairSet result;

    for (int i=0; i<20; i++) {
        const std::string path = "m/44'/0'/0'/0/" + std::to_string(i);
        derive_keys_p2pkh(seed, path, result);
        printf("%s | %s\n", path.c_str(), result.wif_compressed_pubkey.c_str());
    }

#if 0
    for (int i=0; i<20; i++) {
        const std::string path = "m/49'/0'/0'/0/" + std::to_string(i);
        derive_keys_p2sh_p2wpkh(seed, path, result);
        printf("%s | %s\n", path.c_str(), result.wif_compressed_pubkey.c_str());
    }
    for (int i=0; i<20; i++) {
        const std::string path = "m/84'/0'/0'/0/" + std::to_string(i);
        derive_keys_p2wpkh(seed, path, result);
        printf("%s | %s\n", path.c_str(), result.wif_compressed_pubkey.c_str());
    }
#endif
}
