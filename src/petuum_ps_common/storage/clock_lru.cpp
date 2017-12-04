#include <petuum_ps_common/storage/clock_lru.hpp>

namespace petuum {

const int32_t ClockLRU::MAX_NUM_ROUNDS = 20;

ClockLRU::ClockLRU(int capacity, size_t lock_pool_size) :
  capacity_(capacity), evict_hand_(0), insert_hand_(0),
  locks_(lock_pool_size),
  stale_(new std::atomic_flag[capacity]),
  row_ids_(capacity) {
    for (int i = 0; i < capacity_; ++i) {
      // Default constructor for atomic_flag initialize to unspecified state.
      stale_[i].test_and_set();
      row_ids_[i] = -1;
    }
  }

RowId ClockLRU::FindOneToEvict() {
  for (int i = 0; i < MAX_NUM_ROUNDS * capacity_; ++i) {
    // Check slot pointed by evict_hand_ and increment it so other thread will
    // not check this slot immediately.
    int32_t slot = evict_hand_++ % capacity_;

    CHECK(slot >=0 && slot < capacity_) << __func__
                                        << " num_rounds = " << i / capacity_
                                        << " slot = " << slot
                                        << " MAX_NUM_ROUNDS = " << MAX_NUM_ROUNDS;

    // Check recency.
    if (!stale_[slot].test_and_set()) {
      // slot is recent. Set it to stale and skip it.
      continue;
    }
    // We've found a slot that is not recently used (stale).

    // If we can't get lock, then move on.
    Unlocker<SpinMutex> unlocker;
    if (!locks_.TryLock(slot, &unlocker)) {
      continue;
    }
    // stale_[slot] can still be changed after locking, but we don't care.

    // Check occupancy of the slot.
    if (row_ids_[slot] == -1) {
      // Got an empty slot. Duh...
      continue;
    }

    // Found it! Release the lock from unlocker to keep the lock on. slot will
    // be unlocked in Evict() or NoEvict().
    unlocker.Release();
    return row_ids_[slot];
  }
  LOG(FATAL) << "Cannot find a slot to evict after the clock hand goes "
    << MAX_NUM_ROUNDS << " rounds.";
}

void ClockLRU::Evict(int32_t slot) {
  // We assume we are holding lock on the slot.
  row_ids_[slot] = -1;
  empty_slots_.push(slot);
  locks_.Unlock(slot);
}

void ClockLRU::NoEvict(int32_t slot) {
  locks_.Unlock(slot);
}

int32_t ClockLRU::Insert(RowId row_id) {
  Unlocker<SpinMutex> unlocker;
  int32_t slot = FindEmptySlot(&unlocker);
  stale_[slot].clear();
  row_ids_[slot] = row_id;
  return slot;
}

void ClockLRU::Reference(int32_t slot) {
  stale_[slot].clear();
}

int32_t ClockLRU::FindEmptySlot(
    Unlocker<SpinMutex> *unlocker) {
  CHECK_NOTNULL(unlocker);
  // Check empty_slots_.
  int32_t slot = -1;
  if (empty_slots_.pop(&slot) > 0) {
    // empty_slots_ has something.
    // This lock should eventually suceed.
    Unlocker<SpinMutex> tmp_unlocker;
    locks_.Lock(slot, &tmp_unlocker);
    CHECK_EQ(-1, row_ids_[slot]) << "insert_hand_ cannot take slot "
      << "from empty_slots_ queue. Report bug.";
    // Transfer lock
    unlocker->SetLock(tmp_unlocker.GetAndRelease());
    return slot;
  }

  // Going around the clock to look for space is only used in the first round.
  // Afterwards (insert_hand_val_ >= capacity) we know we are out of space.
  int32_t insert_hand_val = insert_hand_++;
  if (insert_hand_val >= capacity_) {
    LOG(FATAL) << "Exceeding ClockLRU capacity. Report Bug";
  }
  // The first round will always succeed at insert_hand_val.
  slot = insert_hand_val % capacity_;
  // Lock to ensure slot stays empty.
  Unlocker<SpinMutex> tmp_unlocker;
  locks_.Lock(slot, &tmp_unlocker);
  CHECK_EQ(-1, row_ids_[slot])
    << "This is first round and must be empty. Report bug.";
  // Transfer lock
  unlocker->SetLock(tmp_unlocker.GetAndRelease());
  return slot;
}

bool ClockLRU::HasRow(RowId row_id, int32_t slot) {
  return (row_ids_[slot] == row_id);
}

}  // namespace petuum
