/*
 * $Id$
 * 
 * Copyright (c) 2015-2018, Luca Fulchir<luker@fenrirproject.org>,
 * All rights reserved.
 *
 * This file is part of "libRaptorQ".
 *
 * libRaptorQ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 *
 * libRaptorQ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and a copy of the GNU Lesser General Public License
 * along with libRaptorQ.  If not, see <http://www.gnu.org/licenses/>.
 */

#define SYMBOL_SIZE 62
#define BLOCK_SIZE  (40*1024)

#include ".git.h"

#ifndef VERSION
#define VERSION "-"
#endif

#include "Timer.hpp"
#include "RandomDrop.hpp"

#include "../src/RaptorQ/RaptorQ_v1_hdr.hpp"

#include <endian.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <stdlib.h>
#include <vector>
#include <cassert>

// rename the main namespace for ease of use
namespace RaptorQ = RaptorQ__v1;

using Binary = std::vector<uint8_t>;
using symbol_id = uint32_t; // just a better name
// we will store here all encoded and transmitted symbols
// std::pair<symbol id (esi), symbol data>
using Symbol = std::pair<symbol_id, Binary>;
using Symbols = std::vector<Symbol>;

using Encoder = RaptorQ::Encoder<typename Binary::iterator, typename Binary::iterator>;
using Decoder = RaptorQ::Decoder<typename Binary::iterator, typename Binary::iterator>;

//-------------------------------------------------------------------------
// size is bytes.
static Binary  generate_random_data(uint32_t size)
{
    Binary input;
    input.reserve (size);

	std::mt19937_64 &rnd = RandomDrop::get_random_generator();
    std::uniform_int_distribution<int16_t> distr (0, std::numeric_limits<uint8_t>::max());
	for (size_t idx = 0; idx < size; ++idx) {
        input.push_back (static_cast<uint8_t> (distr(rnd)));
    }
	return input;
}
//-------------------------------------------------------------------------
static RaptorQ::Block_Size calc_symbols_per_block(size_t block_length, const uint16_t symbol_size)
{
    // how many symbols do we need to encode all our input in a single block?
    auto min_symbols = (block_length * sizeof(uint8_t)) / symbol_size;
    if ((block_length * sizeof(uint8_t)) % symbol_size != 0)
        ++min_symbols;
    // convert "symbols" to a typesafe equivalent, RaptorQ::Block_Size
    // This is needed becouse not all numbers are valid block sizes, and this
    // helps you choose the right block size
    RaptorQ::Block_Size block = RaptorQ::Block_Size::Block_10;
    for (auto blk : *RaptorQ::blocks) {
        // RaptorQ::blocks is a pointer to an array, just scan it to find your
        // block.
        if (static_cast<uint16_t> (blk) >= min_symbols) {
            block = blk;
            break;
        }
    }
	return block;
}
//-------------------------------------------------------------------------
// the "overhead" variable tells us how many symbols more than the
// minimum we will generate. RaptorQ can not always decode a block,
// but there is a small probability that it will fail.
// More overhead => less probability of failure
//  overhead 0 => 1% failures
//  overhead 1 => 0.01% failures
//  overhead 2 => 0.0001% failures
// etc... as you can see, it make little sense to work with more than 3-4
// overhead symbols, but at least one should be considered
static Symbols encode_block(Encoder &enc, Binary &input, uint32_t drop_probability = 5)
{
	const size_t overhead = 4;
	Symbols encoded;
	RandomDrop drop(drop_probability);
	uint32_t drop_data_cnt = 0, drop_repair_cnt = 0;
	
	auto rv = enc.set_data (input.begin(), input.end());
	if(rv != input.size()) {
        std::cerr << "Could not give data to the encoder :(\n";
        return encoded;
	}
	auto symbol_size = enc.symbol_size();
    uint16_t num_symbols = enc.symbols();
	auto real_size = symbol_size * num_symbols;
    std::cerr << "Encode Input block size: " << input.size() 
		<< " symbols: " <<  static_cast<uint32_t> (num_symbols) 
		<< " symbol size: " << static_cast<int32_t>(symbol_size) 
		<< " Real block size: " << real_size
		<< std::endl;
    if (!enc.compute_sync()) {
        // if this happens it's a bug in the library.
        // the **Decoder** can fail, but the **Encoder** can never fail.
        std::cerr << "Enc-RaptorQ failure! really bad!\n";
        return encoded;
    }
	// Now get the source symbols.
	// source symbols are specials because they contain the input data
	// as-is, so if you get all of these, you don't need repair symbols
	for (auto source_sym_it = enc.begin_source(); source_sym_it != enc.end_source(); ++source_sym_it) {
		// make sure the vector has enough space for the symbol:
		// fill it with zeros for the size of the symbol
		Binary source_sym_data (symbol_size, 0);
		// save the data of the symbol into our vector
		auto it = source_sym_data.begin();
		auto written = (*source_sym_it) (it, source_sym_data.end());
		assert(written == symbol_size);
		if(drop()) {
		//if(true) {
			drop_data_cnt++;
			continue;
		}
		symbol_id tmp_id = (*source_sym_it).id();
		encoded.emplace_back (tmp_id, std::move(source_sym_data));
	}
	//--------------------------------------------
	// we finished working with the source symbols.
	// now we need to transmit the repair symbols.
	auto repair_sym_it = enc.begin_repair();
	auto max_repair = enc.max_repair(); // RaptorQ can theoretically handle
										// infinite repair symbols
										// but computers are not so infinite
	// we need to have at least enc.symbols() + overhead symbols.
	for (; encoded.size() < (enc.symbols() + overhead) &&
							repair_sym_it != enc.end_repair (max_repair);
														++repair_sym_it) {
		// make sure the vector has enough space for the symbol:
		// fill it with zeros for the size of the symbol
		Binary repair_sym_data (symbol_size, 0);
		// save the data of the symbol into our vector
		auto it = repair_sym_data.begin();
		auto written = (*repair_sym_it) (it, repair_sym_data.end());
		assert(written == symbol_size);
		if(drop()) {
			drop_repair_cnt++;
			continue;
		}
		symbol_id tmp_id = (*repair_sym_it).id();
		//std::cerr << tmp_id << std::endl;
		encoded.emplace_back (tmp_id, std::move(repair_sym_data));
	}
	if (repair_sym_it == enc.end_repair (enc.max_repair())) {
		// we dropped waaaay too many symbols!
		// should never happen in real life. it means that we do not
		// have enough repair symbols.
		// at this point you can actually start to retransmit the
		// repair symbols from enc.begin_repair(), but we don't care in
		// this example
		std::cerr << "Maybe losing " << drop_probability << "% is too much?\n";
		return Symbols{};
	}
	std::cerr << "Dropped " << drop_data_cnt << " data and " <<  drop_repair_cnt << " repair symbols, " 
		<< drop_probability << "%" << std::endl;
	
	return encoded;
}
//-------------------------------------------------------------------------
#if 0
static Symbols reduction(const Symbols &input, uint16_t num_data_symbols, uint32_t drop_probability)
{
	assert(input.size()>0);

	Symbols output;
	RandomDrop drop(drop_probability);
	uint32_t drop_data_cnt = 0, drop_repair_cnt = 0;
	for (auto it = input.begin(); it != input.end(); ++it) {
		// can we keep this symbol or do we randomly drop it?
		if (!drop()) {
			symbol_id id = (*it).first;
			id>=num_data_symbols?drop_repair_cnt++:drop_data_cnt++;
		} else {
			output.push_back(*it);
		}
	}
	auto percent = (drop_data_cnt + drop_repair_cnt)*100/input.size();
	std::cerr << "Removed " << drop_data_cnt << " data and " <<  drop_repair_cnt << " repair symbols, " 
		<< drop_probability << "=>" << percent << "%" << std::endl;
	return output;
}
#endif
//-------------------------------------------------------------------------
static Binary decode_block(Decoder &dec, Symbols &received, size_t data_size)
{
	Binary decoded;
	uint16_t num_symbols = dec.symbols();
	auto symbol_size = dec.symbol_size();
    std::cerr << "Decode " << received.size() << " symbols from " 
		<< static_cast<uint32_t> (num_symbols) 
		<< ", symbol size: " << static_cast<int32_t>(symbol_size)
		<< " Data size: " << data_size
		<< std::endl;
    // now push every received symbol into the decoder
    for (auto &rec_sym : received) {
        // as a reminder:
        //  rec_sym.first = symbol_id (uint32_t)
        //  rec_sym.second = std::vector<uint8_t> symbol_data
        symbol_id tmp_id = rec_sym.first;
		auto it = rec_sym.second.begin();
		auto err = dec.add_symbol (it, rec_sym.second.end(), tmp_id);
        if (err != RaptorQ::Error::NONE) {
			if(err == RaptorQ::Error::NOT_NEEDED) {
				std::cerr << "NOT_NEEDED" << std::endl;
				break;
			}
            // When you add a symbol, you can get:
            //   NONE: no error
            //   NOT_NEEDED: libRaptorQ ignored it because everything is
            //              already decoded
            //   INITIALIZATION: wrong parameters to the decoder contructor
            //   WRONG_INPUT: not enough data on the symbol?
            //   some_other_error: errors in the library
            std::cerr << "error adding: " << static_cast<unsigned>(err) <<  std::endl;
            return decoded;
        }
	}
	// by now we now there will be no more input, so we tell this to the
	// decoder. You can skip this call, but if the decoder does not have
	// enough data it sill wait forever (or until you call .stop())
	dec.end_of_input (RaptorQ::Fill_With_Zeros::NO);
	bool can_decode = dec.can_decode();
	auto needed_symbols = dec.needed_symbols();
	std::cerr << "Decoder can_decode: " << can_decode << " needed_symbols: " << needed_symbols << std::endl;
	if(!can_decode) {
        std::cerr << "Couldn't decode!\n";
        return decoded;
	}
    auto res = dec.wait_sync();
    if (res.error != RaptorQ::Error::NONE) {
        std::cerr << "Decoding FAILED!.\n";
        return decoded;
    }
	// now save the decoded data in our output
    size_t decode_from_byte = 0;
    size_t skip_bytes_at_begining_of_output = 0;
	decoded.resize(data_size,0);
    auto out_it = decoded.begin();
    auto decode_result = dec.decode_bytes (out_it, decoded.end(), decode_from_byte, skip_bytes_at_begining_of_output);
    // "decode_from_byte" can be used to have only a part of the output.
    // it can be used in advanced setups where you ask only a part
    // of the block at a time.
    // "skip_bytes_at_begining_of_output" is used when dealing with containers
    // which size does not align with the output. For really advanced usage only
    // Both should be zero for most setups.

	if (decode_result.written != data_size) {
        if (decode_result.written == 0) {
            // we were really unlucky and the RQ algorithm needed
            // more symbols!
            std::cerr << "Couldn't decode, RaptorQ Algorithm failure. Can't Retry!\n";
        } else {
            // probably a library error
            std::cerr << "Partial Decoding? This should not have happened: " <<
                                    decode_result.written  << " vs " << data_size << "\n";
        }
        return Binary{};
    } else {
        std::cerr << "Decoded: " << data_size << "\n";
    }
	return decoded;
}
//-------------------------------------------------------------------------
// mysize is bytes.
// the "overhead" variable tells us how many symbols more than the
// minimum we will generate. RaptorQ can not always decode a block,
// but there is a small probability that it will fail.
// More overhead => less probability of failure
//  overhead 0 => 1% failures
//  overhead 1 => 0.01% failures
//  overhead 2 => 0.0001% failures
// etc... as you can see, it make little sense to work with more than 3-4
// overhead symbols, but at least one should be considered
bool test_rq (const uint32_t mysize, const uint16_t symbol_size)
{
	RaptorQ::Block_Size symbols_per_block = calc_symbols_per_block(mysize,symbol_size);

    Timer time(3);
	bool ok = false;
	for(auto i =0; i<5; i++) {
		Encoder enc (symbols_per_block, symbol_size);
		Decoder dec (symbols_per_block, symbol_size, Decoder::Report::COMPLETE);
		ok = false;
		std::cerr << "Generate random data..." << std::endl;
		Binary input = generate_random_data(mysize);
#if 0
		std::cerr << "Precompute start" << std::endl;
		time.start();
		enc.precompute_sync();
		std::cerr << "Precompute: " << time.stop_sec() << " seconds elapsed" << std::endl;
#endif		
		time.start();
		std::cerr << "Encoding start" << std::endl;
		Symbols encoded = encode_block(enc,input,10);
		std::cerr << "Encoded " << encoded.size() << " symbols total, " << time.stop_sec() << " seconds elapsed" << std::endl;
		if(encoded.size() == 0) {
			break;
		}
		//Symbols received = reduction(encoded,enc.symbols(),20);
		std::cerr << "Decoding start" << std::endl;
		time.start();
		Binary decoded = decode_block(dec,encoded,mysize);
		if(decoded.empty()) {
			break;
		}
		std::cerr << "Decoded " << encoded.size() << " symbols total, " << time.stop_sec() << " seconds elapsed" << std::endl;
		bool ok = std::equal(input.begin(),input.end(),decoded.begin());
		std::cerr << "Compare result: " << (ok?"OK":"FAILED!") << std::endl << std::endl;
		if(!ok) {
			break;
		}
	}
    return ok;
}
//-------------------------------------------------------------------------
static bool benchmark()
{
    std::cerr << "Raptor benchmark test" << std::endl;
	// keep some computation in memory. If you use only one block size it
    // will make things faster on bigger block size.
    // allocate 5Mb
    //RaptorQ__v1::local_cache_size (5000000);
	RaptorQ__v1::local_cache_size (0);

    // for our test, we use an input of random size, between 100 and 10.000
    // bytes.
    //std::uniform_int_distribution<uint32_t> distr(100, 10000);
    //uint32_t input_size = distr (rnd);
	const uint32_t input_size = BLOCK_SIZE;
	const uint16_t symbol_size = SYMBOL_SIZE;

    if (!test_rq (input_size, symbol_size))
        return false;
    std::cerr << "The example completed successfully\n";
    return true;
}
//-------------------------------------------------------------------------
namespace std {
	static std::ostream& operator<<(std::ostream& os, const Symbol &sym)
	{
		uint32_t tmp_id_le = htole32(sym.first);
		os.write(reinterpret_cast<const char *>(&tmp_id_le), sizeof(tmp_id_le));
		const Binary & data = sym.second;
		os.write(reinterpret_cast<const char *>(data.data()), data.size());
		return os;
	}
}
//-------------------------------------------------------------------------
static bool symbols2file(const std::string fname, const Symbols &data)
{
	std::ofstream out_file;
	out_file.open (fname, std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
	if (!out_file.is_open()) {
		std::cerr << "ERR: can't open output encoded symbols file for writing\n";
		return false;
	}
	std::ostream_iterator<Symbol> out_it (out_file);
	std::copy(data.begin(),data.end(),out_it);
	out_file.close();
	return true;
}
//-------------------------------------------------------------------------
static bool bin2file(const std::string fname, const Binary &data)
{
	std::ofstream out_file;
	out_file.open (fname, std::ios_base::binary | std::ios_base::out
													| std::ios_base::trunc);
	if (!out_file.is_open()) {
		std::cerr << "ERR: can't open output binary file for writing\n";
		return false;
	}
	std::ostream_iterator<uint8_t> out_it (out_file);
	std::copy(data.begin(),data.end(),out_it);
	out_file.close();
	return true;
}
//-------------------------------------------------------------------------
static bool encode()
{
   	const uint32_t block_size = BLOCK_SIZE;
	const uint16_t symbol_size = SYMBOL_SIZE;

	std::cerr << "Raptor encode test" << std::endl;
	std::cerr << "Generate random data..." << std::endl;
	Binary input = generate_random_data(block_size);
	std::cerr << "Write random data to file..." << std::endl;
	if( !bin2file("raptor_test_data.bin",input)!=0 ) {
		return false;
	}
	
	RaptorQ__v1::local_cache_size (0);
	RaptorQ::Block_Size symbols_per_block = calc_symbols_per_block(block_size,symbol_size);
	Encoder enc (symbols_per_block, symbol_size);

	
	Timer time(3);
	time.start();
	std::cerr << "Encoding start" << std::endl;
	Symbols encoded = encode_block(enc,input,10);
	std::cerr << "Encoded " << encoded.size() << " symbols total, " << time.stop_sec() << " seconds elapsed" << std::endl;
	if(encoded.size() == 0) {
		return false;
	}
	
	std::cerr << "Write encoded symbols..." << std::endl;
	if( symbols2file("raptor_test_data." + std::to_string(symbol_size) + ".enc",encoded)!=0 ) {
		return false;
	}
	std::cerr << "Done!" << std::endl;
	return true;
}
//-------------------------------------------------------------------------
static void usage(const std::string pname)
{
	std::cerr << "Usage: " << std::endl << pname << " encode|decode|benchmark" << std::endl;
}
//-------------------------------------------------------------------------
// https://ru.stackoverflow.com/questions/7920/switch-%D0%B4%D0%BB%D1%8F-string/7924
// FNV-1a hash, 32-bit 
inline constexpr std::uint32_t fnv1a(const char* str, std::uint32_t hash = 2166136261UL) {
    return *str ? fnv1a(str + 1, (hash ^ *str) * 16777619ULL) : hash;
}
//-------------------------------------------------------------------------
static bool process_command(char **argv) 
{
	bool rv = false;
	const std::string cmd = std::string (argv[1]).substr(0,3);
	switch (fnv1a(cmd.c_str())) {
		case fnv1a("ben"):
			rv = benchmark();
			break;
		case fnv1a("enc"):
			rv = encode();
			break;
		case fnv1a("dec"):
			std::cout << "Decode\n"; 
			break;
		default:
			usage(std::string (argv[0]));
	}
	return rv;
}
//-------------------------------------------------------------------------
int main (int argc, char **argv)
{
	std::cerr << "Raptor benchmark " VERSION " Compiled: " __DATE__ " " __TIME__ << std::endl;
	bool rv =  false;
	if(argc>1) {
		rv = process_command(argv);
	} else {
		usage(std::string (argv[0]));
	}
	return rv?0:1;
}
//-------------------------------------------------------------------------
