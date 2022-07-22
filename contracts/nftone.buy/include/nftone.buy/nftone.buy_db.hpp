#pragma once

#include <eosio/asset.hpp>
#include <eosio/privileged.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>

#include <amax.ntoken/amax.ntoken_db.hpp>
#include <utils.hpp>

// #include <deque>
#include <optional>
#include <string>
#include <map>
#include <set>
#include <type_traits>



namespace amax {

using namespace std;
using namespace eosio;

#define HASH256(str) sha256(const_cast<char*>(str.c_str()), str.size())

#define TBL struct [[eosio::table, eosio::contract("nftone.buy")]]
#define NTBL(name) struct [[eosio::table(name), eosio::contract("nftone.buy")]]

NTBL("global") global_t {
    name admin;
    name fee_collector;
    float fee_rate      = 0.0;
    float creator_fee_rate  = 0.0;
    float ipowner_fee_rate  = 0.0;
    float notary_fee_rate   = 0.0;
    eosio::symbol           pay_symbol;
    name                    bank_contract;
    uint64_t last_buy_order_idx = 0;
    uint64_t last_deal_idx      = 0;

    EOSLIB_SERIALIZE( global_t, (admin)(fee_collector)(fee_rate)(creator_fee_rate)(ipowner_fee_rate)
                                (notary_fee_rate)(pay_symbol)(bank_contract)(last_buy_order_idx)(last_deal_idx) )

};
typedef eosio::singleton< "global"_n, global_t > global_singleton;


// price for NFT tokens
struct price_s {
    asset value;    //price value
    nsymbol symbol;

    price_s() {}
    price_s(const asset& v, const nsymbol& symb): value(v), symbol(symb) {}

    friend bool operator > ( const price_s& a, const price_s& b ) {
        return a.value > b.value;
    }

    friend bool operator <= ( const price_s& a, const price_s& b ) {
        return a.value <= b.value;
    }

    friend bool operator < ( const price_s& a, const price_s& b ) {
        return a.value < b.value;
    }

    EOSLIB_SERIALIZE( price_s, (value)(symbol) )
};


//Scope: nasset.symbol.id
TBL order_t {
    uint64_t        id;                 //PK
    price_s         price;
    int64_t         frozen;             //nft amount for sellers
    int64_t         total;
    asset           fee;
    name            maker;
    time_point_sec  begin_at;
    time_point_sec  end_at;
    time_point_sec  created_at;
    time_point_sec  updated_at;

    order_t() {}
    order_t(const uint64_t& i): id(i) {}

    uint64_t primary_key()const { return id; }

    uint64_t by_small_price_first()const { return price.value.amount; }
    uint64_t by_large_price_first()const { return( std::numeric_limits<uint64_t>::max() - price.value.amount ); }
    uint128_t by_maker_small_price_first()const { return (uint128_t) maker.value << 64 | (uint128_t) price.value.amount ; }
    uint128_t by_maker_large_price_first()const { return (uint128_t) maker.value << 64 | (uint128_t) (std::numeric_limits<uint64_t>::max() - price.value.amount ); }
    uint128_t by_maker_created_at()const { return (uint128_t) maker.value << 64 | (uint128_t) created_at.sec_since_epoch(); }

    EOSLIB_SERIALIZE( order_t, (id)(price)(frozen)(total)(fee)(maker)(begin_at)(end_at)(created_at)(updated_at) )

};

TBL buyer_bid_t {
    uint64_t        id;
    uint64_t        sell_order_id;
    price_s         price;
    asset           frozen; //CNYD
    name            buyer;
    time_point_sec  created_at;

    buyer_bid_t() {}
    buyer_bid_t(const uint64_t& i): id(i) {}

    uint64_t primary_key()const { return id; }

    uint64_t by_large_price_first()const { return( std::numeric_limits<uint64_t>::max() - price.value.amount ); }

    checksum256 by_buyer_created_at()const { return make256key( sell_order_id,
                                                                buyer.value,
                                                                created_at.sec_since_epoch(),
                                                                id); }
    uint64_t by_sell_order_id()const { return sell_order_id; }

    EOSLIB_SERIALIZE( buyer_bid_t, (id)(sell_order_id)(price)(frozen)(buyer)(created_at) )

    typedef eosio::multi_index
    < "buyerbids"_n,  buyer_bid_t,
        indexed_by<"priceidx"_n,        const_mem_fun<buyer_bid_t, uint64_t, &buyer_bid_t::by_large_price_first> >,
        indexed_by<"sellorderidx"_n,    const_mem_fun<buyer_bid_t, uint64_t, &buyer_bid_t::by_sell_order_id> >,
        indexed_by<"createidx"_n,       const_mem_fun<buyer_bid_t, checksum256, &buyer_bid_t::by_buyer_created_at> >
    > idx_t;
};


typedef eosio::multi_index
< "sellorders"_n,  order_t,
    indexed_by<"makerordidx"_n,     const_mem_fun<order_t, uint128_t, &order_t::by_maker_small_price_first> >,
    indexed_by<"makercreated"_n,    const_mem_fun<order_t, uint128_t, &order_t::by_maker_created_at> >,
    indexed_by<"priceidx"_n,        const_mem_fun<order_t, uint64_t,  &order_t::by_small_price_first> >
> sellorder_idx;


} //namespace amax