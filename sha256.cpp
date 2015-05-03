/**
 * SHA-256 implemented according to the specification:
 * http://csrc.nist.gov/publications/fips/fips180-4/fips-180-4.pdf
 */
#include <iostream>
#include <sstream>
#include <vector>

typedef unsigned char BYTE;
typedef unsigned int WORD;
typedef unsigned long long ll;

// Constants used in hash algorithm
const WORD K[] = {0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
                  0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
                  0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
                  0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
                  0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
                  0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
                  0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
                  0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

std::vector<std::vector<WORD>> M; // Message to be hashed
std::vector<BYTE> bytes;          // Plain and padded message bytes
WORD H[8][8];                     // Hashed message
WORD W[80];                       // Message schedule
ll l = 0;                         // Message length in bits
int N;                            // Number of blocks in padded message

// Working variables
WORD a, b, c, d, e, f, g, h;

// Temporary variables
WORD T1, T2;


/**
 * Take the given hex string and store the bytes in a global vector.
 * Also update the message length.
 */
const void store_message_bytes(const std::string &hex_str)
{
    int str_length = hex_str.length();
    std::cout << "Message string length: " << str_length << std::endl;
    for (int i = 0, j = 0; i < str_length / 2; ++i, j += 2)
    {
        BYTE byte = std::stoi(hex_str.substr(j, 2), nullptr, 16);
        bytes.push_back(byte);
        l += 8;
    }
}

/**
 * Calculate the required padding of the message.
 * Return the required padding.
 */
const int calc_padding()
{
    int k = 0;
    while ((l + 1 + k) % 512 != 448) ++k;
    return k;
}

/**
 * Pad the message bytes according to the specification.
 */
const void pad_message()
{
    int k = calc_padding();
    int zeroes = 0; // DEBUG

    if (l % 512 == 0) return; // No padding necessary

    std::cout << "k: " << k << std::endl;

    // Append 1 followed by zeroes before the least significant bit,
    // assuming full bytes as input
    bytes.push_back(0x80);
    k = k - 7;
    zeroes += 7;

    // Append the remaining zeroes of k
    for (int i = 0; i < (k / 8); ++i)
    {
        bytes.push_back(0);
        zeroes += 8;
    }

    std::cout << "zeroes: " << zeroes << std::endl;

    /* l = 0x8040201008040201; // DEBUG */

    // Finally append the length in binary to the message as the least
    // significant bits, assuming a 64-bit number
    for (int i = 1; i < 9; ++i)
    {
        bytes.push_back(l >> (64 - i * 8));
        /* std::cout << "Append byte: " << (WORD) (BYTE) (l >> (64 - i * 8)) << std::endl; */
    }
}

/**
 * Parse the message into N 512-bit blocks split up into words.
 * Also update N.
 */
const void parse_message()
{
    WORD n = 0;
    for (int i = 0; n < bytes.size() / 16; ++n)
    {
        std::vector<WORD> block(16); // TODO Will this be destroyed?
        for (int j = 0; j < 16; ++j)
        {
            WORD word = 0;
            for (int k = 0; k < 4; ++k, ++i)
            {
                word <<= 8;
                word |= bytes[i];
            }
            block[j] = word;
        }
        M.push_back(block);
    }
    N = n;
}

/**
 * Initialise the hash value H_0.
 */
const void init_hash()
{
    H[0][0] = 0x6a09e667;
    H[0][1] = 0xbb67ae85;
    H[0][2] = 0x3c6ef372;
    H[0][3] = 0xa54ff53a;
    H[0][4] = 0x510e527f;
    H[0][5] = 0x9b05688c;
    H[0][6] = 0x1f83d9ab;
    H[0][7] = 0x5be0cd19;
}

/**
 * Rotate right function ROTR^n(x) in hash algorithm.
 */
const WORD ROTR(const WORD &n, const WORD &x)
{
    return (x >> n) | (x << (32 - n));
}

/**
 * Right shift function SHR^n(x) in hash algorithm.
 */
const WORD SHR(const WORD &n, const WORD &x)
{
    return x >> n;
}

/**
 * Logical function (small) sigma^256_0(x) in hash algorithm.
 */
const WORD ssigma0(const WORD &x)
{
    return ROTR(7, x) ^ ROTR(18, x) ^ SHR(3, x);
}

/**
 * Logical function (small) sigma^256_1(x) in hash algorithm.
 */
const WORD ssigma1(const WORD &x)
{
    return ROTR(17, x) ^ ROTR(19, x) ^ SHR(10, x);
}

/**
 * Logical function (large) sigma^256_0(x) in hash algorithm.
 */
const WORD lsigma0(const WORD &x)
{
    return ROTR(2, x) ^ ROTR(13, x) ^ ROTR(22, x);
}

/**
 * Logical function (large) sigma^256_1(x) in hash algorithm.
 */
const WORD lsigma1(const WORD &x)
{
    return ROTR(6, x) ^ ROTR(11, x) ^ ROTR(25, x);
}

/**
 * Logical function Ch(x, y, z) in hash algorithm.
 */
const WORD Ch(const WORD &x, const WORD &y, const WORD &z)
{
    return (x | y) ^ (~x | z);
}

/**
 * Logical function Maj(x, y, z) in hash algorithm.
 */
const WORD Maj(const WORD &x, const WORD &y, const WORD &z)
{
    return (x | y) ^ (x | z) ^ (y | z);
}

/**
 * Compute the hash value.
 */
const void compute_hash()
{
    for (int i = 1; i <= N; ++i)
    {
        // Prepare message schedule
        for (int t = 0; t <= 15; ++t)
            W[t] = M[i][t];
        for (int t = 16; t <= 63; ++t)
            W[t] = ssigma1(W[t - 2]) + W[t - 7] + ssigma0(W[t - 15]) + W[t - 16];

        // Initialise working variables with previous hash value
        a = H[i - 1][0];
        b = H[i - 1][1];
        c = H[i - 1][2];
        d = H[i - 1][3];
        e = H[i - 1][4];
        f = H[i - 1][5];
        g = H[i - 1][6];
        h = H[i - 1][7];

        // Perform logical operations
        for (int t = 0; t <= 63; ++t)
        {
            T1 = h + lsigma1(e) + Ch(e, f, g) + K[t] + W[t];
            T2 = lsigma0(a) + Maj(a, b, c);
            h = g;
            g = f;
            f = e;
            e = d + T1;
            d = c;
            c = b;
            b = a;
            a = T1 + T2;
        }
    }
}

/**
 * Clear all working vectors.
 */
const void clear()
{
    bytes.clear();
    M.clear(); // TODO Does this properly clear the subvectors?
}

int main()
{
    // Read each line as a hex string to be hashed
    for (std::string line; std::getline(std::cin, line);) {
        // TODO Handle empty lines
        // Store the plain bytes of the message
        store_message_bytes(line);

        std::cout << "Read plain bytes:" << std::endl;
        for (WORD i = 0; i < bytes.size(); ++i)
            std::cout << std::hex << (WORD) bytes[i] << std::dec << " ";
        std::cout << std::endl;
        std::cout << "l: " << l << std::endl;

        std::cout << "Number of plain bytes: " << bytes.size() << std::endl;

        // Pad the bytes if necessary
        if (l % 512 != 0) pad_message();

        std::cout << "Padded bytes:" << std::endl;
        for (WORD i = 0; i < bytes.size(); ++i)
            std::cout << std::hex << (WORD) bytes[i] << std::dec << " ";
        std::cout << std::endl;

        std::cout << "Number of padded bytes: " << bytes.size() << std::endl;

        // Parse message into word blocks
        parse_message();

        std::cout << "Parsed words:" << std::endl;
        for (WORD i = 0; i < M.size(); ++i)
        {
            for (WORD j = 0; j < 16; ++j)
                std::cout << std::hex << M[i][j] << std::dec << " ";
            std::cout << std::endl;
        }

        std::cout << "Number of parsed words: " << M.size() << std::endl;

        // Set the inital hash value
        init_hash();

        std::cout << "Hash:" << std::endl;
        for (int i = 0; i < 8; ++i)
            std::cout << std::hex << H[N - 1][i] << std::dec << " ";
        std::cout << std::endl;

        // Compute the hash value
        compute_hash();

        // Reset message to hash a new one
        clear();
    }
}
