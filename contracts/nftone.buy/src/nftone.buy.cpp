#include <nftone.buy/nftone.buy.hpp>
#include <amax.ntoken/amax.ntoken_db.hpp>
#include <amax.ntoken/amax.ntoken.hpp>
#include <cnyd.token/amax.xtoken.hpp>
#include <utils.hpp>

static constexpr eosio::name active_permission{"active"_n};


namespace amax {

using namespace std;

#define CHECKC(exp, code, msg) \
   { if (!(exp)) eosio::check(false, string("$$$") + to_string((int)code) + string("$$$ ") + msg); }


   inline int64_t get_precision(const symbol &s) {
      int64_t digit = s.precision();
      CHECK(digit >= 0 && digit <= 18, "precision digit " + std::to_string(digit) + " should be in range[0,18]");
      return calc_precision(digit);
   }


   void nftone_mart::init(eosio::symbol pay_symbol, name bank_contract) {
      require_auth( _self );

      _gstate.admin                 = "amax.daodev"_n;
      _gstate.fee_collector         = "amax.daodev"_n;
      _gstate.pay_symbol            = pay_symbol;
      _gstate.bank_contract         = bank_contract;

      //reset with default
      // _gstate = global_t{};
   }

   /**
    * @brief send nasset tokens into nftone marketplace
    *
    * @param from
    * @param to
    * @param quantity
    * @param memo: $ask_price      E.g.:  10288    (its currency unit is CNYD)
    *
    */
   void nftone_mart::onselltransfer(const name& from, const name& to, const vector<nasset>& quants, const string& memo) {
      CHECKC( from != to, err::ACCOUNT_INVALID, "cannot transfer to self" );

      if (from == get_self() || to != get_self()) return;

      CHECKC( memo != "", err::MEMO_FORMAT_ERROR, "empty memo!" )
      CHECKC( quants.size() == 1, err::OVERSIZED, "only one nft allowed to sell to nft at a timepoint" )
      asset price          = asset( 0, _gstate.pay_symbol );
      compute_memo_price( memo, price );

      auto quant              = quants[0];
      CHECKC( quant.amount > 0, err::PARAM_ERROR, "non-positive quantity not allowed" )
      auto ask_price          = price_s(price, quant.symbol);

      auto sellorders = sellorder_idx( _self, quant.symbol.id );
      _gstate.last_buy_order_idx ++;
      sellorders.emplace(_self, [&]( auto& row ) {
         row.id         =  _gstate.last_buy_order_idx;
         row.price      = ask_price;
         row.frozen     = quant.amount;
         row.total      = quant.amount;
         row.maker      = from;
         row.created_at = time_point_sec( current_time_point() );
      });
   }

   void nftone_mart::setorderfee(const uint64_t& order_id, const uint64_t& token_id,
                           const time_point_sec& begin_at,
                           const time_point_sec& end_at, 
                           const asset& fee) {
      require_auth( _self );
      CHECKC( begin_at > current_time_point(), err::PARAM_ERROR, "current time is not greater than begin" );
      CHECKC( begin_at < end_at, err::PARAM_ERROR, "begin is not greater than end" );
      CHECKC( fee.amount > 0, err::PARAM_ERROR, "non-positive quantity not allowed" );

      auto orders       = sellorder_idx( _self, token_id );
      auto itr          = orders.find( order_id );
      CHECKC( itr != orders.end(), err::RECORD_NOT_FOUND, "order not found: " + to_string(order_id) + "@" + to_string(token_id) )

      orders.modify(itr, same_payer, [&]( auto& row ) {
         row.begin_at    = begin_at;
         row.end_at      = end_at;
         row.fee         = fee;
      });

   }

   void nftone_mart::onbuytransfercnyd(const name& from, const name& to, const asset& quant, const string& memo) {
      on_buy_transfer(from, to, quant, memo);
   }

   void nftone_mart::onbuytransfermtoken(const name& from, const name& to, const asset& quant, const string& memo) {
      on_buy_transfer(from, to, quant, memo);
   }

   /**
    * @brief send CNYD tokens into nftone marketplace to buy or place buy order
    *
    * @param from
    * @param to
    * @param quant
    * @param memo: $token_id:$order_id:count
    *       E.g.:  123:1:10288
    */
   void nftone_mart::on_buy_transfer(const name& from, const name& to, const asset& quant, const string& memo) {

      CHECKC( quant.symbol == _gstate.pay_symbol, err::SYMBOL_MISMATCH, "pay symbol not matched")
      if (from == get_self() || to != get_self()) return;

      CHECKC( from != to, err::ACCOUNT_INVALID, "cannot transfer to self" );
      CHECKC( quant.amount > 0, err::PARAM_ERROR, "non-positive quantity not allowed" )
      CHECKC( memo != "", err::MEMO_FORMAT_ERROR, "empty memo!" )

      vector<string_view> params = split(memo, ":");
      auto param_size            = params.size();
      CHECKC( param_size == 3, err::MEMO_FORMAT_ERROR, "memo format incorrect" )

      // memo zero
      auto token_id           = to_uint64( params[0], "token_id" );
      auto nstats             = nstats_t::idx_t(NFT_BANK, NFT_BANK.value);
      auto nstats_itr         = nstats.find(token_id);
      CHECKC( nstats_itr != nstats.end(), err::RECORD_NOT_FOUND, "nft token not found: " + to_string(token_id) )

      auto token_pid          = nstats_itr->supply.symbol.parent_id;
      auto nsymb              = nsymbol( token_id, token_pid );
      // memo two
      int64_t count           = stoi(string(params[2]));
      CHECKC( count > 0, err::PARAM_ERROR, "non-positive count not allowed" )
      auto bought             = nasset(count, nsymb); //by buyer

      auto orders             = sellorder_idx( _self, token_id );
      // memo one
      auto order_id           = stoi( string( params[1] ));
      auto itr                = orders.find( order_id );
      CHECKC( itr != orders.end(), err::RECORD_NOT_FOUND, "order not found: " + to_string(order_id) + "@" + to_string(token_id) )

      auto order = *itr;
      CHECKC( time_point_sec( current_time_point() ) > order.begin_at, err::TIME_EXPIRED, "There is no buying at this time" )
      CHECKC( time_point_sec( current_time_point() ) < order.end_at, err::TIME_EXPIRED, "There is no buying at this time" )
      CHECKC( count <= order.frozen, err::PARAM_ERROR, "count cannot exceed the remaining quantity" )
   
      uint64_t need_amount = (order.price.value.amount + order.fee.amount) * count;
      CHECKC( need_amount > 0, err::PARAM_ERROR, "non-positive count not allowed" )
      CHECKC( quant.amount == need_amount, err::INCORRECT_AMOUNT, "incorrect amount" )

      auto earned    = asset(0, _gstate.pay_symbol); //to seller
      earned.amount  = count * order.price.value.amount;
      order.frozen  -= count;
      //send to seller for quote tokens
      TRANSFER_X( _gstate.bank_contract, order.maker, earned, "sell nft:" + to_string(bought.symbol.id) )

      if ( order.frozen == 0 ) {
         orders.erase( itr );
      } else {
         orders.modify(itr, same_payer, [&]( auto& row ) {
            row.frozen = order.frozen;
            row.updated_at = current_time_point();
         });
      }

      _on_deal_trace( order.id, 0, order.maker, from,
                     order.price, order.fee, count, current_time_point() );

      //send to buyer for nft tokens
      vector<nasset> quants = { bought };
      TRANSFER_N( NFT_BANK, from, quants, "buy nft: " + to_string(token_id) )

      //send to fee_collector for fee
      uint64_t need_fee = order.fee.amount * count;
      auto fee_asset = asset(need_fee, _gstate.pay_symbol); //to seller
      TRANSFER_X( _gstate.bank_contract, _gstate.fee_collector, fee_asset, "NFT " + to_string(bought.symbol.id) + " handling fee, count:"+ to_string(count))

   }

   void nftone_mart::cancelorder(const name& maker, const uint32_t& token_id, const uint64_t& order_id) {
      require_auth( maker );

      auto orders = sellorder_idx(_self, token_id);
      if (order_id != 0) {
         auto itr = orders.find( order_id );
         CHECKC( itr != orders.end(), err::RECORD_NOT_FOUND, "order not exit: " + to_string(order_id) + "@" + to_string(token_id) )
         CHECKC( maker == itr->maker, err::NO_AUTH, "NO_AUTH")

         auto nft_quant = nasset( itr->frozen, itr->price.symbol );
         vector<nasset> quants = { nft_quant };
         TRANSFER_N( NFT_BANK, itr->maker, quants, "nftone mart cancel" )
         orders.erase( itr );

      } else {
         for (auto itr = orders.begin(); itr != orders.end(); itr++) {
            auto nft_quant = nasset( itr->frozen, itr->price.symbol );
            vector<nasset> quants = { nft_quant };
            TRANSFER_N( NFT_BANK, itr->maker, quants, "nftone mart cancel" )
            orders.erase( itr );
         }
      }
   }

   void nftone_mart::compute_memo_price(const string& memo, asset& price) {
      price.amount =  to_int64( memo, "price");
      CHECKC( price.amount > 0, err::PARAM_ERROR, " price non-positive quantity not allowed" )
   }

   void nftone_mart::dealtrace(const uint64_t& seller_order_id,
                     const uint64_t& bid_id,
                     const name& seller,
                     const name& buyer,
                     const price_s& price,
                     const asset& fee,
                     const int64_t& count,
                     const time_point_sec created_at
                   )
   {
      require_auth(get_self());
      require_recipient(seller);
      require_recipient(buyer);
   }

   void nftone_mart::_on_deal_trace(const uint64_t& seller_order_id,
                     const uint64_t&   bid_id,
                     const name&       maker,
                     const name&       buyer,
                     const price_s&    price,
                     const asset&      fee,
                     const int64_t     count,
                     const time_point_sec created_at)
    {
         amax::nftone_mart::deal_trace_action act{ _self, { {_self, active_permission} } };
			act.send( seller_order_id, bid_id, maker, buyer, price, fee, count, created_at );

   }

} //namespace amax