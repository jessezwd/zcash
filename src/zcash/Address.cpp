
#include "Address.hpp"
#include "NoteEncryption.hpp"
#include "hash.h"
#include "prf.h"
#include "streams.h"

#include <librustzcash.h>

namespace libzcash {

uint256 SproutPaymentAddress::GetHash() const {
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << *this;
    return Hash(ss.begin(), ss.end());
}

uint256 ReceivingKey::pk_enc() const {
    return ZCNoteEncryption::generate_pubkey(*this);
}

SproutPaymentAddress SproutViewingKey::address() const {
    return SproutPaymentAddress(a_pk, sk_enc.pk_enc());
}

ReceivingKey SproutSpendingKey::receiving_key() const {
    return ReceivingKey(ZCNoteEncryption::generate_privkey(*this));
}

SproutViewingKey SproutSpendingKey::viewing_key() const {
    return SproutViewingKey(PRF_addr_a_pk(*this), receiving_key());
}

SproutSpendingKey SproutSpendingKey::random() {
    return SproutSpendingKey(random_uint252());
}

SproutPaymentAddress SproutSpendingKey::address() const {
    return viewing_key().address();
}

//! Sapling
SaplingFullViewingKey SaplingExpandedSpendingKey::full_viewing_key() const {
    uint256 ak;
    uint256 nk;
    librustzcash_ask_to_ak(ask.begin(), ak.begin());
    librustzcash_nsk_to_nk(nsk.begin(), nk.begin());
    return SaplingFullViewingKey(ak, nk, ovk);
}

SaplingExpandedSpendingKey SaplingSpendingKey::expanded_spending_key() const {
    return SaplingExpandedSpendingKey(PRF_ask(*this), PRF_nsk(*this), PRF_ovk(*this));
}

SaplingFullViewingKey SaplingSpendingKey::full_viewing_key() const {
    return expanded_spending_key().full_viewing_key();
}

SaplingIncomingViewingKey SaplingFullViewingKey::in_viewing_key() const {
    uint256 ivk;
    librustzcash_crh_ivk(ak.begin(), nk.begin(), ivk.begin());
    return SaplingIncomingViewingKey(ivk);
}

SaplingSpendingKey SaplingSpendingKey::random() {
    return SaplingSpendingKey(random_uint256());
}

boost::optional<SaplingPaymentAddress> SaplingIncomingViewingKey::address(diversifier_t d) const {
    uint256 pk_d;
    if (librustzcash_check_diversifier(d.data())) {
        librustzcash_ivk_to_pkd(this->begin(), d.data(), pk_d.begin());
        return SaplingPaymentAddress(d, pk_d);
    } else {
        return boost::none;
    }
}

SaplingPaymentAddress SaplingSpendingKey::default_address() const {
    // Iterates within default_diversifier to ensure a valid address is returned
    auto addrOpt = full_viewing_key().in_viewing_key().address(default_diversifier(*this));
    assert(addrOpt != boost::none);
    return addrOpt.value();
}

}


bool IsValidPaymentAddress(const libzcash::PaymentAddress& zaddr) {
    return zaddr.which() != 0;
}

bool IsValidViewingKey(const libzcash::ViewingKey& vk) {
    return vk.which() != 0;
}

bool IsValidSpendingKey(const libzcash::SpendingKey& zkey) {
    return zkey.which() != 0;
}
