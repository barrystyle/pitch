#ifndef MNEMONIC_H
#define MNEMONIC_H

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <base58.h>
#include <mnemonic.h>

#include <openssl/sha.h>
#include <string>
#include <vector>

#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include <cstdint>

void entropy_to_mnemonic(char* entropy_in, int bitlen, std::string& mnemonic);
void mnemonic_to_seed(std::string& phrase, std::string passphrase, std::vector<uint8_t>& seed_out);
void calculate_from_seed(std::vector<uint8_t>& seed);

bool derive_keys_p2pkh(std::vector<uint8_t> seed_vec, const std::string path, pairSet& result);
bool derive_keys_p2sh_p2wpkh(std::vector<uint8_t> seed_vec, const std::string path, pairSet& result);
bool derive_keys_p2wpkh(std::vector<uint8_t> seed_vec, const std::string path, pairSet& result);

#endif // MNEMONIC_H
