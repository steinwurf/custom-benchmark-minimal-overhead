// Copyright (c) Steinwurf ApS 2017.
// All Rights Reserved
//
// Distributed under the "BSD License". See the accompanying LICENSE.rst file.

// Copyright (c) 2012 Steinwurf ApS
// All Rights Reserved
//
// Distributed under the "BSD License". See the accompanying LICENSE.rst file.

#include <algorithm>
#include <numeric>
#include <ctime>
#include <string>

#include <gauge/gauge.hpp>
#include <kodo_rlnc/seed_codes.hpp>

// This test shows how you can add options to the benchmark
// options
template<class Encoder, class Decoder>
class benchmark : public gauge::time_benchmark
{
public:

    benchmark()
    {
        std::vector<uint32_t> symbols = {10240};//, 102400};
        std::vector<uint32_t> symbol_size = {1040};
        std::vector<std::string> type = {"encoder", "decoder"};

        for (const auto& s: symbols)
        {
            for (const auto& ss: symbol_size)
            {
                for (const auto& t: type)
                {
                    gauge::config_set cs;
                    cs.set_value<uint32_t>("symbols", s);
                    cs.set_value<uint32_t>("symbol_size", ss);
                    cs.set_value<std::string>("type", t);

                    gauge::time_benchmark::add_configuration(cs);
                }
            }
        }

        srand(static_cast<uint32_t>(time(0)));

    }


    void setup() override
    {
        // tbd
    }

    void test_body() override
    {
        gauge::config_set c = get_current_configuration();
        std::string type = c.get_value<std::string>("type");

        if (type == "encoder")
        {
            run_encoder();
        }
        else if (type == "decoder")
        {
            run_decoder();
        }
    }

    void run_encoder()
    {
        gauge::config_set cs = gauge::time_benchmark::get_current_configuration();

        using id_type = uint32_t;

        uint32_t symbols = cs.get_value<uint32_t>("symbols");
        uint32_t symbol_size = cs.get_value<uint32_t>("symbol_size");

        typename Encoder::factory encoder_factory(symbols, symbol_size);
        auto encoder = encoder_factory.build();

        std::vector<uint8_t> coefficients(encoder->coefficient_vector_size());
        std::vector<uint8_t> data_in(encoder->block_size());

        std::generate(data_in.begin(), data_in.end(), rand);

        encoder->set_const_symbols(storage::storage(data_in));

        assert(symbols*3 <= std::numeric_limits<id_type>::max());

        std::vector<std::vector<uint8_t>> encoded_payloads(
            symbols*3, std::vector<uint8_t>(sizeof(id_type) + encoder->symbol_size()));

        // Clock ticks
        RUN
        {
            id_type seed = 0;
            for (auto& payload : encoded_payloads)
            {
                // Set the seed to use
                encoder->set_seed(seed);

                // Generate an encoding vector
                encoder->generate(coefficients.data());

                // Positions of the "symbol id" and "symbol data" field
                // in the payload buffer
                uint8_t* seed_dst = payload.data();
                uint8_t* symbol_dst = payload.data() + sizeof(id_type);

                // Write the seed value (in big endian)
                endian::big_endian::put<id_type>(seed, seed_dst);

                // Write a symbol according to the generated coefficients
                encoder->write_symbol(symbol_dst, coefficients.data());

                // Increment the seed for each iteration of the loop
                ++seed;
            }
        }
    }

    void run_decoder()
    {
        gauge::config_set cs = gauge::time_benchmark::get_current_configuration();

        using id_type = uint32_t;

        uint32_t symbols = cs.get_value<uint32_t>("symbols");
        uint32_t symbol_size = cs.get_value<uint32_t>("symbol_size");

        typename Encoder::factory encoder_factory(symbols, symbol_size);
        auto encoder = encoder_factory.build();

        std::vector<uint8_t> coefficients(encoder->coefficient_vector_size());
        std::vector<uint8_t> data_in(encoder->block_size());

        std::generate(data_in.begin(), data_in.end(), rand);

        encoder->set_const_symbols(storage::storage(data_in));

        assert(symbols*3 <= std::numeric_limits<id_type>::max());

        std::vector<std::vector<uint8_t>> encoded_payloads(
            symbols*3, std::vector<uint8_t>(sizeof(id_type) + encoder->symbol_size()));

        // Clock ticks
        id_type seed = 0;
        for (auto& payload : encoded_payloads)
        {
            // Set the seed to use
            encoder->set_seed(seed);

            // Generate an encoding vector
            encoder->generate(coefficients.data());

            // Positions of the "symbol id" and "symbol data" field
            // in the payload buffer
            uint8_t* seed_dst = payload.data();
            uint8_t* symbol_dst = payload.data() + sizeof(id_type);

            // Write the seed value (in big endian)
            endian::big_endian::put<id_type>(seed, seed_dst);

            // Write a symbol according to the generated coefficients
            encoder->write_symbol(symbol_dst, coefficients.data());

            // Increment the seed for each iteration of the loop
            ++seed;
        }

        typename Decoder::factory decoder_factory(symbols, symbol_size);
        auto decoder = decoder_factory.build();

        RUN
        {
            for (auto& payload : encoded_payloads)
            {
                // Positions of the "symbol id" and "symbol data" field
                // in the payload buffer
                uint8_t* seed_src = payload.data();
                uint8_t* symbol_src = payload.data() + sizeof(id_type);

                // Read the seed value
                id_type seed = endian::big_endian::get<id_type>(seed_src);

                // Set the seed to use
                decoder->set_seed(seed);

                // Generate an encoding vector - we reuse the buffer
                decoder->generate(coefficients.data());

                // Read a symbol according to the generated coefficients
                decoder->read_symbol(symbol_src, coefficients.data());

                if (decoder->is_complete())
                {
                    break;
                }
            }
        }
    }

};

using seed_codec_binary = benchmark<
    kodo_rlnc::seed_encoder<fifi::binary>,
    kodo_rlnc::seed_decoder<fifi::binary>>;

BENCHMARK_F(seed_codec_binary, SeedRLNC, Binary, 1);

using seed_codec_binary4 = benchmark<
    kodo_rlnc::seed_encoder<fifi::binary4>,
    kodo_rlnc::seed_decoder<fifi::binary4>>;

BENCHMARK_F(seed_codec_binary4, SeedRLNC, Binary4, 1);

using seed_codec_binary8 = benchmark<
    kodo_rlnc::seed_encoder<fifi::binary8>,
    kodo_rlnc::seed_decoder<fifi::binary8>>;

BENCHMARK_F(seed_codec_binary8, SeedRLNC, Binary8, 1);

using seed_codec_binary16 = benchmark<
    kodo_rlnc::seed_encoder<fifi::binary16>,
    kodo_rlnc::seed_decoder<fifi::binary16>>;

BENCHMARK_F(seed_codec_binary16, SeedRLNC, Binary16, 1);


int main(int argc, const char* argv[])
{
    srand((uint32_t)time(0));

    gauge::runner::add_default_printers();
    gauge::runner::run_benchmarks(argc, argv);
    return 0;
}
