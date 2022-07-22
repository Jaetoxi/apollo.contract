#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/permission.hpp>
#include <eosio/action.hpp>

#include <string>

#include <nftone.buy/nftone.buy_db.hpp>

namespace amax {

using std::string;
using std::vector;

using namespace eosio;

static constexpr name      NFT_BANK    = "amax.ntoken"_n;
static constexpr name      CNYD_BANK   = "cnyd.token"_n;
static constexpr symbol    CNYD        = symbol(symbol_code("CNYD"), 4);

enum class err: uint8_t {
   NONE                 = 0,
   RECORD_NOT_FOUND     = 1,
   RECORD_EXISTING      = 2,
   SYMBOL_MISMATCH      = 4,
   PARAM_ERROR          = 5,
   MEMO_FORMAT_ERROR    = 6,
   PAUSED               = 7,
   NO_AUTH              = 8,
   NOT_POSITIVE         = 9,
   NOT_STARTED          = 10,
   OVERSIZED            = 11,
   TIME_EXPIRED         = 12,
   NOTIFY_UNRELATED     = 13,
   ACTION_REDUNDANT     = 14,
   ACCOUNT_INVALID      = 15,
   FEE_INSUFFICIENT     = 16,
   FIRST_CREATOR        = 17,
   STATUS_ERROR         = 18,
   INCORRECT_AMOUNT     = 19
};


/**
 * The `nftone.buy` sample system contract defines the structures and actions that allow users to create, issue, and manage tokens for AMAX based blockchains. It demonstrates one way to implement a smart contract which allows for creation and management of tokens. It is possible for one to create a similar contract which suits different needs. However, it is recommended that if one only needs a token with the below listed actions, that one uses the `nftone.buy` contract instead of developing their own.
 *
 * The `nftone.buy` contract class also implements two useful public static methods: `get_supply` and `get_balance`. The first allows one to check the total supply of a specified token, created by an account and the second allows one to check the balance of a token for a specified account (the token creator account has to be specified as well).
 *
 * The `nftone.buy` contract manages the set of tokens, accounts and their corresponding balances, by using two internal multi-index structures: the `accounts` and `stats`. The `accounts` multi-index table holds, for each row, instances of `account` object and the `account` object holds information about the balance of one token. The `accounts` table is scoped to an eosio account, and it keeps the rows indexed based on the token's symbol.  This means that when one queries the `accounts` multi-index table for an account name the result is all the tokens that account holds at the moment.
 *
 * Similarly, the `stats` multi-index table, holds instances of `currency_stats` objects for each row, which contains information about current supply, maximum supply, and the creator account for a symbol token. The `stats` table is scoped to the token symbol.  Therefore, when one queries the `stats` table for a token symbol the result is one single entry/row corresponding to the queried symbol token if it was previously created, or nothing, otherwise.
 */
class [[eosio::contract("nftone.buy")]] nftone_mart : public contract {
   public:
      using contract::contract;

   nftone_mart(eosio::name receiver, eosio::name code, datastream<const char*> ds): contract(receiver, code, ds),
        _global(get_self(), get_self().value)
    {
        _gstate = _global.exists() ? _global.get() : global_t{};
    }

    ~nftone_mart() { _global.set( _gstate, get_self() ); }

   [[eosio::on_notify("amax.ntoken::transfer")]]
   void onselltransfer(const name& from, const name& to, const vector<nasset>& quants, const string& memo);

   ACTION setorderfee(const uint64_t& order_id, const uint64_t& token_id, const time_point_sec& begin_at, const time_point_sec& end_at, const asset& fee);

   [[eosio::on_notify("cnyd.token::transfer")]]
   void onbuytransfercnyd(const name& from, const name& to, const asset& quant, const string& memo);

   [[eosio::on_notify("amax.mtoken::transfer")]]
   void onbuytransfermtoken(const name& from, const name& to, const asset& quant, const string& memo);

   ACTION init(eosio::symbol symbol, name bank_contract);

   ACTION cancelorder(const name& maker, const uint32_t& token_id, const uint64_t& order_id);

   ACTION dealtrace(const uint64_t& seller_order_id,
                     const uint64_t& bid_id,
                     const name& seller,
                     const name& buyer,
                     const price_s& price,
                     const asset& fee,
                     const int64_t& count,
                     const time_point_sec created_at
                   );

   using deal_trace_action = eosio::action_wrapper<"dealtrace"_n, &nftone_mart::dealtrace>;

   private:
      global_singleton    _global;
      global_t            _gstate;

   private:

      void compute_memo_price( const string& memo, asset& price );

      void on_buy_transfer(const name& from, const name& to, const asset& quant, const string& memo);

      void _on_deal_trace(const uint64_t& seller_order_id,
                     const uint64_t& buy_order_id,
                     const name& seller,
                     const name& buyer,
                     const price_s& price,
                     const asset& fee,
                     const int64_t count,
                     const time_point_sec created_at);

};
} //namespace amax
