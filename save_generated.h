// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_SAVE_SAVE_H_
#define FLATBUFFERS_GENERATED_SAVE_SAVE_H_

#include "flatbuffers/flatbuffers.h"

namespace Save {

struct PerishCounter;

struct CombatStat;

struct MaterialInfo;

struct Material;

struct Good;

struct AI;

struct Traveler;

struct Business;

struct Town;

struct Game;

FLATBUFFERS_MANUALLY_ALIGNED_STRUCT(4) PerishCounter FLATBUFFERS_FINAL_CLASS {
 private:
  int16_t time_;
  int16_t padding0__;
  float amount_;

 public:
  PerishCounter() {
    memset(this, 0, sizeof(PerishCounter));
  }
  PerishCounter(int16_t _time, float _amount)
      : time_(flatbuffers::EndianScalar(_time)),
        padding0__(0),
        amount_(flatbuffers::EndianScalar(_amount)) {
    (void)padding0__;
  }
  int16_t time() const {
    return flatbuffers::EndianScalar(time_);
  }
  float amount() const {
    return flatbuffers::EndianScalar(amount_);
  }
};
FLATBUFFERS_STRUCT_END(PerishCounter, 8);

FLATBUFFERS_MANUALLY_ALIGNED_STRUCT(2) CombatStat FLATBUFFERS_FINAL_CLASS {
 private:
  int16_t statId_;
  int16_t partId_;
  int16_t attack_;
  int16_t type_;
  int16_t speed_;
  int16_t bashDefense_;
  int16_t cutDefense_;
  int16_t stabDefense_;

 public:
  CombatStat() {
    memset(this, 0, sizeof(CombatStat));
  }
  CombatStat(int16_t _statId, int16_t _partId, int16_t _attack, int16_t _type, int16_t _speed, int16_t _bashDefense, int16_t _cutDefense, int16_t _stabDefense)
      : statId_(flatbuffers::EndianScalar(_statId)),
        partId_(flatbuffers::EndianScalar(_partId)),
        attack_(flatbuffers::EndianScalar(_attack)),
        type_(flatbuffers::EndianScalar(_type)),
        speed_(flatbuffers::EndianScalar(_speed)),
        bashDefense_(flatbuffers::EndianScalar(_bashDefense)),
        cutDefense_(flatbuffers::EndianScalar(_cutDefense)),
        stabDefense_(flatbuffers::EndianScalar(_stabDefense)) {
  }
  int16_t statId() const {
    return flatbuffers::EndianScalar(statId_);
  }
  int16_t partId() const {
    return flatbuffers::EndianScalar(partId_);
  }
  int16_t attack() const {
    return flatbuffers::EndianScalar(attack_);
  }
  int16_t type() const {
    return flatbuffers::EndianScalar(type_);
  }
  int16_t speed() const {
    return flatbuffers::EndianScalar(speed_);
  }
  int16_t bashDefense() const {
    return flatbuffers::EndianScalar(bashDefense_);
  }
  int16_t cutDefense() const {
    return flatbuffers::EndianScalar(cutDefense_);
  }
  int16_t stabDefense() const {
    return flatbuffers::EndianScalar(stabDefense_);
  }
};
FLATBUFFERS_STRUCT_END(CombatStat, 16);

FLATBUFFERS_MANUALLY_ALIGNED_STRUCT(8) MaterialInfo FLATBUFFERS_FINAL_CLASS {
 private:
  double limitFactor_;
  double minPrice_;
  double maxPrice_;
  double value_;
  double buy_;
  double sell_;

 public:
  MaterialInfo() {
    memset(this, 0, sizeof(MaterialInfo));
  }
  MaterialInfo(double _limitFactor, double _minPrice, double _maxPrice, double _value, double _buy, double _sell)
      : limitFactor_(flatbuffers::EndianScalar(_limitFactor)),
        minPrice_(flatbuffers::EndianScalar(_minPrice)),
        maxPrice_(flatbuffers::EndianScalar(_maxPrice)),
        value_(flatbuffers::EndianScalar(_value)),
        buy_(flatbuffers::EndianScalar(_buy)),
        sell_(flatbuffers::EndianScalar(_sell)) {
  }
  double limitFactor() const {
    return flatbuffers::EndianScalar(limitFactor_);
  }
  double minPrice() const {
    return flatbuffers::EndianScalar(minPrice_);
  }
  double maxPrice() const {
    return flatbuffers::EndianScalar(maxPrice_);
  }
  double value() const {
    return flatbuffers::EndianScalar(value_);
  }
  double buy() const {
    return flatbuffers::EndianScalar(buy_);
  }
  double sell() const {
    return flatbuffers::EndianScalar(sell_);
  }
};
FLATBUFFERS_STRUCT_END(MaterialInfo, 48);

struct Material FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum {
    VT_ID = 4,
    VT_NAME = 6,
    VT_AMOUNT = 8,
    VT_CONSUMPTION = 10,
    VT_DEMANDSLOPE = 12,
    VT_DEMANDINTERCEPT = 14,
    VT_PERISHCOUNTERS = 16,
    VT_COMBATSTATS = 18,
    VT_LIMITFACTOR = 20
  };
  int16_t id() const {
    return GetField<int16_t>(VT_ID, 0);
  }
  const flatbuffers::String *name() const {
    return GetPointer<const flatbuffers::String *>(VT_NAME);
  }
  float amount() const {
    return GetField<float>(VT_AMOUNT, 0.0f);
  }
  float consumption() const {
    return GetField<float>(VT_CONSUMPTION, 0.0f);
  }
  float demandSlope() const {
    return GetField<float>(VT_DEMANDSLOPE, 0.0f);
  }
  float demandIntercept() const {
    return GetField<float>(VT_DEMANDINTERCEPT, 0.0f);
  }
  const flatbuffers::Vector<const PerishCounter *> *perishCounters() const {
    return GetPointer<const flatbuffers::Vector<const PerishCounter *> *>(VT_PERISHCOUNTERS);
  }
  const flatbuffers::Vector<const CombatStat *> *combatStats() const {
    return GetPointer<const flatbuffers::Vector<const CombatStat *> *>(VT_COMBATSTATS);
  }
  float limitFactor() const {
    return GetField<float>(VT_LIMITFACTOR, 0.0f);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<int16_t>(verifier, VT_ID) &&
           VerifyOffset(verifier, VT_NAME) &&
           verifier.VerifyString(name()) &&
           VerifyField<float>(verifier, VT_AMOUNT) &&
           VerifyField<float>(verifier, VT_CONSUMPTION) &&
           VerifyField<float>(verifier, VT_DEMANDSLOPE) &&
           VerifyField<float>(verifier, VT_DEMANDINTERCEPT) &&
           VerifyOffset(verifier, VT_PERISHCOUNTERS) &&
           verifier.VerifyVector(perishCounters()) &&
           VerifyOffset(verifier, VT_COMBATSTATS) &&
           verifier.VerifyVector(combatStats()) &&
           VerifyField<float>(verifier, VT_LIMITFACTOR) &&
           verifier.EndTable();
  }
};

struct MaterialBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_id(int16_t id) {
    fbb_.AddElement<int16_t>(Material::VT_ID, id, 0);
  }
  void add_name(flatbuffers::Offset<flatbuffers::String> name) {
    fbb_.AddOffset(Material::VT_NAME, name);
  }
  void add_amount(float amount) {
    fbb_.AddElement<float>(Material::VT_AMOUNT, amount, 0.0f);
  }
  void add_consumption(float consumption) {
    fbb_.AddElement<float>(Material::VT_CONSUMPTION, consumption, 0.0f);
  }
  void add_demandSlope(float demandSlope) {
    fbb_.AddElement<float>(Material::VT_DEMANDSLOPE, demandSlope, 0.0f);
  }
  void add_demandIntercept(float demandIntercept) {
    fbb_.AddElement<float>(Material::VT_DEMANDINTERCEPT, demandIntercept, 0.0f);
  }
  void add_perishCounters(flatbuffers::Offset<flatbuffers::Vector<const PerishCounter *>> perishCounters) {
    fbb_.AddOffset(Material::VT_PERISHCOUNTERS, perishCounters);
  }
  void add_combatStats(flatbuffers::Offset<flatbuffers::Vector<const CombatStat *>> combatStats) {
    fbb_.AddOffset(Material::VT_COMBATSTATS, combatStats);
  }
  void add_limitFactor(float limitFactor) {
    fbb_.AddElement<float>(Material::VT_LIMITFACTOR, limitFactor, 0.0f);
  }
  explicit MaterialBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  MaterialBuilder &operator=(const MaterialBuilder &);
  flatbuffers::Offset<Material> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<Material>(end);
    return o;
  }
};

inline flatbuffers::Offset<Material> CreateMaterial(
    flatbuffers::FlatBufferBuilder &_fbb,
    int16_t id = 0,
    flatbuffers::Offset<flatbuffers::String> name = 0,
    float amount = 0.0f,
    float consumption = 0.0f,
    float demandSlope = 0.0f,
    float demandIntercept = 0.0f,
    flatbuffers::Offset<flatbuffers::Vector<const PerishCounter *>> perishCounters = 0,
    flatbuffers::Offset<flatbuffers::Vector<const CombatStat *>> combatStats = 0,
    float limitFactor = 0.0f) {
  MaterialBuilder builder_(_fbb);
  builder_.add_limitFactor(limitFactor);
  builder_.add_combatStats(combatStats);
  builder_.add_perishCounters(perishCounters);
  builder_.add_demandIntercept(demandIntercept);
  builder_.add_demandSlope(demandSlope);
  builder_.add_consumption(consumption);
  builder_.add_amount(amount);
  builder_.add_name(name);
  builder_.add_id(id);
  return builder_.Finish();
}

inline flatbuffers::Offset<Material> CreateMaterialDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    int16_t id = 0,
    const char *name = nullptr,
    float amount = 0.0f,
    float consumption = 0.0f,
    float demandSlope = 0.0f,
    float demandIntercept = 0.0f,
    const std::vector<PerishCounter> *perishCounters = nullptr,
    const std::vector<CombatStat> *combatStats = nullptr,
    float limitFactor = 0.0f) {
  return Save::CreateMaterial(
      _fbb,
      id,
      name ? _fbb.CreateString(name) : 0,
      amount,
      consumption,
      demandSlope,
      demandIntercept,
      perishCounters ? _fbb.CreateVectorOfStructs<PerishCounter>(*perishCounters) : 0,
      combatStats ? _fbb.CreateVectorOfStructs<CombatStat>(*combatStats) : 0,
      limitFactor);
}

struct Good FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum {
    VT_ID = 4,
    VT_NAME = 6,
    VT_AMOUNT = 8,
    VT_MATERIALS = 10,
    VT_PERISH = 12,
    VT_CARRY = 14,
    VT_MEASURE = 16,
    VT_SHOOTS = 18
  };
  int16_t id() const {
    return GetField<int16_t>(VT_ID, 0);
  }
  const flatbuffers::String *name() const {
    return GetPointer<const flatbuffers::String *>(VT_NAME);
  }
  float amount() const {
    return GetField<float>(VT_AMOUNT, 0.0f);
  }
  const flatbuffers::Vector<flatbuffers::Offset<Material>> *materials() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<Material>> *>(VT_MATERIALS);
  }
  float perish() const {
    return GetField<float>(VT_PERISH, 0.0f);
  }
  float carry() const {
    return GetField<float>(VT_CARRY, 0.0f);
  }
  const flatbuffers::String *measure() const {
    return GetPointer<const flatbuffers::String *>(VT_MEASURE);
  }
  int32_t shoots() const {
    return GetField<int32_t>(VT_SHOOTS, 0);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<int16_t>(verifier, VT_ID) &&
           VerifyOffset(verifier, VT_NAME) &&
           verifier.VerifyString(name()) &&
           VerifyField<float>(verifier, VT_AMOUNT) &&
           VerifyOffset(verifier, VT_MATERIALS) &&
           verifier.VerifyVector(materials()) &&
           verifier.VerifyVectorOfTables(materials()) &&
           VerifyField<float>(verifier, VT_PERISH) &&
           VerifyField<float>(verifier, VT_CARRY) &&
           VerifyOffset(verifier, VT_MEASURE) &&
           verifier.VerifyString(measure()) &&
           VerifyField<int32_t>(verifier, VT_SHOOTS) &&
           verifier.EndTable();
  }
};

struct GoodBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_id(int16_t id) {
    fbb_.AddElement<int16_t>(Good::VT_ID, id, 0);
  }
  void add_name(flatbuffers::Offset<flatbuffers::String> name) {
    fbb_.AddOffset(Good::VT_NAME, name);
  }
  void add_amount(float amount) {
    fbb_.AddElement<float>(Good::VT_AMOUNT, amount, 0.0f);
  }
  void add_materials(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<Material>>> materials) {
    fbb_.AddOffset(Good::VT_MATERIALS, materials);
  }
  void add_perish(float perish) {
    fbb_.AddElement<float>(Good::VT_PERISH, perish, 0.0f);
  }
  void add_carry(float carry) {
    fbb_.AddElement<float>(Good::VT_CARRY, carry, 0.0f);
  }
  void add_measure(flatbuffers::Offset<flatbuffers::String> measure) {
    fbb_.AddOffset(Good::VT_MEASURE, measure);
  }
  void add_shoots(int32_t shoots) {
    fbb_.AddElement<int32_t>(Good::VT_SHOOTS, shoots, 0);
  }
  explicit GoodBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  GoodBuilder &operator=(const GoodBuilder &);
  flatbuffers::Offset<Good> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<Good>(end);
    return o;
  }
};

inline flatbuffers::Offset<Good> CreateGood(
    flatbuffers::FlatBufferBuilder &_fbb,
    int16_t id = 0,
    flatbuffers::Offset<flatbuffers::String> name = 0,
    float amount = 0.0f,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<Material>>> materials = 0,
    float perish = 0.0f,
    float carry = 0.0f,
    flatbuffers::Offset<flatbuffers::String> measure = 0,
    int32_t shoots = 0) {
  GoodBuilder builder_(_fbb);
  builder_.add_shoots(shoots);
  builder_.add_measure(measure);
  builder_.add_carry(carry);
  builder_.add_perish(perish);
  builder_.add_materials(materials);
  builder_.add_amount(amount);
  builder_.add_name(name);
  builder_.add_id(id);
  return builder_.Finish();
}

inline flatbuffers::Offset<Good> CreateGoodDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    int16_t id = 0,
    const char *name = nullptr,
    float amount = 0.0f,
    const std::vector<flatbuffers::Offset<Material>> *materials = nullptr,
    float perish = 0.0f,
    float carry = 0.0f,
    const char *measure = nullptr,
    int32_t shoots = 0) {
  return Save::CreateGood(
      _fbb,
      id,
      name ? _fbb.CreateString(name) : 0,
      amount,
      materials ? _fbb.CreateVector<flatbuffers::Offset<Material>>(*materials) : 0,
      perish,
      carry,
      measure ? _fbb.CreateString(measure) : 0,
      shoots);
}

struct AI FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum {
    VT_DECISIONCOUNTER = 4,
    VT_DECISIONCRITERIA = 6,
    VT_MATERIALINFO = 8
  };
  int16_t decisionCounter() const {
    return GetField<int16_t>(VT_DECISIONCOUNTER, 0);
  }
  const flatbuffers::Vector<float> *decisionCriteria() const {
    return GetPointer<const flatbuffers::Vector<float> *>(VT_DECISIONCRITERIA);
  }
  const flatbuffers::Vector<const MaterialInfo *> *materialInfo() const {
    return GetPointer<const flatbuffers::Vector<const MaterialInfo *> *>(VT_MATERIALINFO);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<int16_t>(verifier, VT_DECISIONCOUNTER) &&
           VerifyOffset(verifier, VT_DECISIONCRITERIA) &&
           verifier.VerifyVector(decisionCriteria()) &&
           VerifyOffset(verifier, VT_MATERIALINFO) &&
           verifier.VerifyVector(materialInfo()) &&
           verifier.EndTable();
  }
};

struct AIBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_decisionCounter(int16_t decisionCounter) {
    fbb_.AddElement<int16_t>(AI::VT_DECISIONCOUNTER, decisionCounter, 0);
  }
  void add_decisionCriteria(flatbuffers::Offset<flatbuffers::Vector<float>> decisionCriteria) {
    fbb_.AddOffset(AI::VT_DECISIONCRITERIA, decisionCriteria);
  }
  void add_materialInfo(flatbuffers::Offset<flatbuffers::Vector<const MaterialInfo *>> materialInfo) {
    fbb_.AddOffset(AI::VT_MATERIALINFO, materialInfo);
  }
  explicit AIBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  AIBuilder &operator=(const AIBuilder &);
  flatbuffers::Offset<AI> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<AI>(end);
    return o;
  }
};

inline flatbuffers::Offset<AI> CreateAI(
    flatbuffers::FlatBufferBuilder &_fbb,
    int16_t decisionCounter = 0,
    flatbuffers::Offset<flatbuffers::Vector<float>> decisionCriteria = 0,
    flatbuffers::Offset<flatbuffers::Vector<const MaterialInfo *>> materialInfo = 0) {
  AIBuilder builder_(_fbb);
  builder_.add_materialInfo(materialInfo);
  builder_.add_decisionCriteria(decisionCriteria);
  builder_.add_decisionCounter(decisionCounter);
  return builder_.Finish();
}

inline flatbuffers::Offset<AI> CreateAIDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    int16_t decisionCounter = 0,
    const std::vector<float> *decisionCriteria = nullptr,
    const std::vector<MaterialInfo> *materialInfo = nullptr) {
  return Save::CreateAI(
      _fbb,
      decisionCounter,
      decisionCriteria ? _fbb.CreateVector<float>(*decisionCriteria) : 0,
      materialInfo ? _fbb.CreateVectorOfStructs<MaterialInfo>(*materialInfo) : 0);
}

struct Traveler FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum {
    VT_NAME = 4,
    VT_TOWN = 6,
    VT_OLDTOWN = 8,
    VT_NATION = 10,
    VT_LOG = 12,
    VT_LONGITUDE = 14,
    VT_LATITUDE = 16,
    VT_GOODS = 18,
    VT_STATS = 20,
    VT_PARTS = 22,
    VT_EQUIPMENT = 24,
    VT_AI = 26,
    VT_MOVING = 28
  };
  const flatbuffers::String *name() const {
    return GetPointer<const flatbuffers::String *>(VT_NAME);
  }
  int16_t town() const {
    return GetField<int16_t>(VT_TOWN, 0);
  }
  int16_t oldTown() const {
    return GetField<int16_t>(VT_OLDTOWN, 0);
  }
  int16_t nation() const {
    return GetField<int16_t>(VT_NATION, 0);
  }
  const flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>> *log() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>> *>(VT_LOG);
  }
  float longitude() const {
    return GetField<float>(VT_LONGITUDE, 0.0f);
  }
  float latitude() const {
    return GetField<float>(VT_LATITUDE, 0.0f);
  }
  const flatbuffers::Vector<flatbuffers::Offset<Good>> *goods() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<Good>> *>(VT_GOODS);
  }
  const flatbuffers::Vector<int16_t> *stats() const {
    return GetPointer<const flatbuffers::Vector<int16_t> *>(VT_STATS);
  }
  const flatbuffers::Vector<int16_t> *parts() const {
    return GetPointer<const flatbuffers::Vector<int16_t> *>(VT_PARTS);
  }
  const flatbuffers::Vector<flatbuffers::Offset<Good>> *equipment() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<Good>> *>(VT_EQUIPMENT);
  }
  const AI *ai() const {
    return GetPointer<const AI *>(VT_AI);
  }
  bool moving() const {
    return GetField<uint8_t>(VT_MOVING, 0) != 0;
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_NAME) &&
           verifier.VerifyString(name()) &&
           VerifyField<int16_t>(verifier, VT_TOWN) &&
           VerifyField<int16_t>(verifier, VT_OLDTOWN) &&
           VerifyField<int16_t>(verifier, VT_NATION) &&
           VerifyOffset(verifier, VT_LOG) &&
           verifier.VerifyVector(log()) &&
           verifier.VerifyVectorOfStrings(log()) &&
           VerifyField<float>(verifier, VT_LONGITUDE) &&
           VerifyField<float>(verifier, VT_LATITUDE) &&
           VerifyOffset(verifier, VT_GOODS) &&
           verifier.VerifyVector(goods()) &&
           verifier.VerifyVectorOfTables(goods()) &&
           VerifyOffset(verifier, VT_STATS) &&
           verifier.VerifyVector(stats()) &&
           VerifyOffset(verifier, VT_PARTS) &&
           verifier.VerifyVector(parts()) &&
           VerifyOffset(verifier, VT_EQUIPMENT) &&
           verifier.VerifyVector(equipment()) &&
           verifier.VerifyVectorOfTables(equipment()) &&
           VerifyOffset(verifier, VT_AI) &&
           verifier.VerifyTable(ai()) &&
           VerifyField<uint8_t>(verifier, VT_MOVING) &&
           verifier.EndTable();
  }
};

struct TravelerBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_name(flatbuffers::Offset<flatbuffers::String> name) {
    fbb_.AddOffset(Traveler::VT_NAME, name);
  }
  void add_town(int16_t town) {
    fbb_.AddElement<int16_t>(Traveler::VT_TOWN, town, 0);
  }
  void add_oldTown(int16_t oldTown) {
    fbb_.AddElement<int16_t>(Traveler::VT_OLDTOWN, oldTown, 0);
  }
  void add_nation(int16_t nation) {
    fbb_.AddElement<int16_t>(Traveler::VT_NATION, nation, 0);
  }
  void add_log(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>> log) {
    fbb_.AddOffset(Traveler::VT_LOG, log);
  }
  void add_longitude(float longitude) {
    fbb_.AddElement<float>(Traveler::VT_LONGITUDE, longitude, 0.0f);
  }
  void add_latitude(float latitude) {
    fbb_.AddElement<float>(Traveler::VT_LATITUDE, latitude, 0.0f);
  }
  void add_goods(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<Good>>> goods) {
    fbb_.AddOffset(Traveler::VT_GOODS, goods);
  }
  void add_stats(flatbuffers::Offset<flatbuffers::Vector<int16_t>> stats) {
    fbb_.AddOffset(Traveler::VT_STATS, stats);
  }
  void add_parts(flatbuffers::Offset<flatbuffers::Vector<int16_t>> parts) {
    fbb_.AddOffset(Traveler::VT_PARTS, parts);
  }
  void add_equipment(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<Good>>> equipment) {
    fbb_.AddOffset(Traveler::VT_EQUIPMENT, equipment);
  }
  void add_ai(flatbuffers::Offset<AI> ai) {
    fbb_.AddOffset(Traveler::VT_AI, ai);
  }
  void add_moving(bool moving) {
    fbb_.AddElement<uint8_t>(Traveler::VT_MOVING, static_cast<uint8_t>(moving), 0);
  }
  explicit TravelerBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  TravelerBuilder &operator=(const TravelerBuilder &);
  flatbuffers::Offset<Traveler> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<Traveler>(end);
    return o;
  }
};

inline flatbuffers::Offset<Traveler> CreateTraveler(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flatbuffers::String> name = 0,
    int16_t town = 0,
    int16_t oldTown = 0,
    int16_t nation = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>> log = 0,
    float longitude = 0.0f,
    float latitude = 0.0f,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<Good>>> goods = 0,
    flatbuffers::Offset<flatbuffers::Vector<int16_t>> stats = 0,
    flatbuffers::Offset<flatbuffers::Vector<int16_t>> parts = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<Good>>> equipment = 0,
    flatbuffers::Offset<AI> ai = 0,
    bool moving = false) {
  TravelerBuilder builder_(_fbb);
  builder_.add_ai(ai);
  builder_.add_equipment(equipment);
  builder_.add_parts(parts);
  builder_.add_stats(stats);
  builder_.add_goods(goods);
  builder_.add_latitude(latitude);
  builder_.add_longitude(longitude);
  builder_.add_log(log);
  builder_.add_name(name);
  builder_.add_nation(nation);
  builder_.add_oldTown(oldTown);
  builder_.add_town(town);
  builder_.add_moving(moving);
  return builder_.Finish();
}

inline flatbuffers::Offset<Traveler> CreateTravelerDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    const char *name = nullptr,
    int16_t town = 0,
    int16_t oldTown = 0,
    int16_t nation = 0,
    const std::vector<flatbuffers::Offset<flatbuffers::String>> *log = nullptr,
    float longitude = 0.0f,
    float latitude = 0.0f,
    const std::vector<flatbuffers::Offset<Good>> *goods = nullptr,
    const std::vector<int16_t> *stats = nullptr,
    const std::vector<int16_t> *parts = nullptr,
    const std::vector<flatbuffers::Offset<Good>> *equipment = nullptr,
    flatbuffers::Offset<AI> ai = 0,
    bool moving = false) {
  return Save::CreateTraveler(
      _fbb,
      name ? _fbb.CreateString(name) : 0,
      town,
      oldTown,
      nation,
      log ? _fbb.CreateVector<flatbuffers::Offset<flatbuffers::String>>(*log) : 0,
      longitude,
      latitude,
      goods ? _fbb.CreateVector<flatbuffers::Offset<Good>>(*goods) : 0,
      stats ? _fbb.CreateVector<int16_t>(*stats) : 0,
      parts ? _fbb.CreateVector<int16_t>(*parts) : 0,
      equipment ? _fbb.CreateVector<flatbuffers::Offset<Good>>(*equipment) : 0,
      ai,
      moving);
}

struct Business FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum {
    VT_ID = 4,
    VT_MODE = 6,
    VT_NAME = 8,
    VT_AREA = 10,
    VT_CANSWITCH = 12,
    VT_KEEPMATERIAL = 14,
    VT_INPUTS = 16,
    VT_OUTPUTS = 18,
    VT_FREQUENCYFACTOR = 20
  };
  int16_t id() const {
    return GetField<int16_t>(VT_ID, 0);
  }
  int16_t mode() const {
    return GetField<int16_t>(VT_MODE, 0);
  }
  const flatbuffers::String *name() const {
    return GetPointer<const flatbuffers::String *>(VT_NAME);
  }
  float area() const {
    return GetField<float>(VT_AREA, 0.0f);
  }
  bool canSwitch() const {
    return GetField<uint8_t>(VT_CANSWITCH, 0) != 0;
  }
  bool keepMaterial() const {
    return GetField<uint8_t>(VT_KEEPMATERIAL, 0) != 0;
  }
  const flatbuffers::Vector<flatbuffers::Offset<Good>> *inputs() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<Good>> *>(VT_INPUTS);
  }
  const flatbuffers::Vector<flatbuffers::Offset<Good>> *outputs() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<Good>> *>(VT_OUTPUTS);
  }
  float frequencyFactor() const {
    return GetField<float>(VT_FREQUENCYFACTOR, 0.0f);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<int16_t>(verifier, VT_ID) &&
           VerifyField<int16_t>(verifier, VT_MODE) &&
           VerifyOffset(verifier, VT_NAME) &&
           verifier.VerifyString(name()) &&
           VerifyField<float>(verifier, VT_AREA) &&
           VerifyField<uint8_t>(verifier, VT_CANSWITCH) &&
           VerifyField<uint8_t>(verifier, VT_KEEPMATERIAL) &&
           VerifyOffset(verifier, VT_INPUTS) &&
           verifier.VerifyVector(inputs()) &&
           verifier.VerifyVectorOfTables(inputs()) &&
           VerifyOffset(verifier, VT_OUTPUTS) &&
           verifier.VerifyVector(outputs()) &&
           verifier.VerifyVectorOfTables(outputs()) &&
           VerifyField<float>(verifier, VT_FREQUENCYFACTOR) &&
           verifier.EndTable();
  }
};

struct BusinessBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_id(int16_t id) {
    fbb_.AddElement<int16_t>(Business::VT_ID, id, 0);
  }
  void add_mode(int16_t mode) {
    fbb_.AddElement<int16_t>(Business::VT_MODE, mode, 0);
  }
  void add_name(flatbuffers::Offset<flatbuffers::String> name) {
    fbb_.AddOffset(Business::VT_NAME, name);
  }
  void add_area(float area) {
    fbb_.AddElement<float>(Business::VT_AREA, area, 0.0f);
  }
  void add_canSwitch(bool canSwitch) {
    fbb_.AddElement<uint8_t>(Business::VT_CANSWITCH, static_cast<uint8_t>(canSwitch), 0);
  }
  void add_keepMaterial(bool keepMaterial) {
    fbb_.AddElement<uint8_t>(Business::VT_KEEPMATERIAL, static_cast<uint8_t>(keepMaterial), 0);
  }
  void add_inputs(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<Good>>> inputs) {
    fbb_.AddOffset(Business::VT_INPUTS, inputs);
  }
  void add_outputs(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<Good>>> outputs) {
    fbb_.AddOffset(Business::VT_OUTPUTS, outputs);
  }
  void add_frequencyFactor(float frequencyFactor) {
    fbb_.AddElement<float>(Business::VT_FREQUENCYFACTOR, frequencyFactor, 0.0f);
  }
  explicit BusinessBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  BusinessBuilder &operator=(const BusinessBuilder &);
  flatbuffers::Offset<Business> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<Business>(end);
    return o;
  }
};

inline flatbuffers::Offset<Business> CreateBusiness(
    flatbuffers::FlatBufferBuilder &_fbb,
    int16_t id = 0,
    int16_t mode = 0,
    flatbuffers::Offset<flatbuffers::String> name = 0,
    float area = 0.0f,
    bool canSwitch = false,
    bool keepMaterial = false,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<Good>>> inputs = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<Good>>> outputs = 0,
    float frequencyFactor = 0.0f) {
  BusinessBuilder builder_(_fbb);
  builder_.add_frequencyFactor(frequencyFactor);
  builder_.add_outputs(outputs);
  builder_.add_inputs(inputs);
  builder_.add_area(area);
  builder_.add_name(name);
  builder_.add_mode(mode);
  builder_.add_id(id);
  builder_.add_keepMaterial(keepMaterial);
  builder_.add_canSwitch(canSwitch);
  return builder_.Finish();
}

inline flatbuffers::Offset<Business> CreateBusinessDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    int16_t id = 0,
    int16_t mode = 0,
    const char *name = nullptr,
    float area = 0.0f,
    bool canSwitch = false,
    bool keepMaterial = false,
    const std::vector<flatbuffers::Offset<Good>> *inputs = nullptr,
    const std::vector<flatbuffers::Offset<Good>> *outputs = nullptr,
    float frequencyFactor = 0.0f) {
  return Save::CreateBusiness(
      _fbb,
      id,
      mode,
      name ? _fbb.CreateString(name) : 0,
      area,
      canSwitch,
      keepMaterial,
      inputs ? _fbb.CreateVector<flatbuffers::Offset<Good>>(*inputs) : 0,
      outputs ? _fbb.CreateVector<flatbuffers::Offset<Good>>(*outputs) : 0,
      frequencyFactor);
}

struct Town FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum {
    VT_ID = 4,
    VT_NAMES = 6,
    VT_NATION = 8,
    VT_LONGITUDE = 10,
    VT_LATITUDE = 12,
    VT_COASTAL = 14,
    VT_POPULATION = 16,
    VT_TOWNTYPE = 18,
    VT_BUSINESSES = 20,
    VT_GOODS = 22,
    VT_NEIGHBORS = 24,
    VT_BUSINESSCOUNTER = 26
  };
  int16_t id() const {
    return GetField<int16_t>(VT_ID, 0);
  }
  const flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>> *names() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>> *>(VT_NAMES);
  }
  int16_t nation() const {
    return GetField<int16_t>(VT_NATION, 0);
  }
  float longitude() const {
    return GetField<float>(VT_LONGITUDE, 0.0f);
  }
  float latitude() const {
    return GetField<float>(VT_LATITUDE, 0.0f);
  }
  bool coastal() const {
    return GetField<uint8_t>(VT_COASTAL, 0) != 0;
  }
  int16_t population() const {
    return GetField<int16_t>(VT_POPULATION, 0);
  }
  int16_t townType() const {
    return GetField<int16_t>(VT_TOWNTYPE, 0);
  }
  const flatbuffers::Vector<flatbuffers::Offset<Business>> *businesses() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<Business>> *>(VT_BUSINESSES);
  }
  const flatbuffers::Vector<flatbuffers::Offset<Good>> *goods() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<Good>> *>(VT_GOODS);
  }
  const flatbuffers::Vector<int16_t> *neighbors() const {
    return GetPointer<const flatbuffers::Vector<int16_t> *>(VT_NEIGHBORS);
  }
  int16_t businessCounter() const {
    return GetField<int16_t>(VT_BUSINESSCOUNTER, 0);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<int16_t>(verifier, VT_ID) &&
           VerifyOffset(verifier, VT_NAMES) &&
           verifier.VerifyVector(names()) &&
           verifier.VerifyVectorOfStrings(names()) &&
           VerifyField<int16_t>(verifier, VT_NATION) &&
           VerifyField<float>(verifier, VT_LONGITUDE) &&
           VerifyField<float>(verifier, VT_LATITUDE) &&
           VerifyField<uint8_t>(verifier, VT_COASTAL) &&
           VerifyField<int16_t>(verifier, VT_POPULATION) &&
           VerifyField<int16_t>(verifier, VT_TOWNTYPE) &&
           VerifyOffset(verifier, VT_BUSINESSES) &&
           verifier.VerifyVector(businesses()) &&
           verifier.VerifyVectorOfTables(businesses()) &&
           VerifyOffset(verifier, VT_GOODS) &&
           verifier.VerifyVector(goods()) &&
           verifier.VerifyVectorOfTables(goods()) &&
           VerifyOffset(verifier, VT_NEIGHBORS) &&
           verifier.VerifyVector(neighbors()) &&
           VerifyField<int16_t>(verifier, VT_BUSINESSCOUNTER) &&
           verifier.EndTable();
  }
};

struct TownBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_id(int16_t id) {
    fbb_.AddElement<int16_t>(Town::VT_ID, id, 0);
  }
  void add_names(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>> names) {
    fbb_.AddOffset(Town::VT_NAMES, names);
  }
  void add_nation(int16_t nation) {
    fbb_.AddElement<int16_t>(Town::VT_NATION, nation, 0);
  }
  void add_longitude(float longitude) {
    fbb_.AddElement<float>(Town::VT_LONGITUDE, longitude, 0.0f);
  }
  void add_latitude(float latitude) {
    fbb_.AddElement<float>(Town::VT_LATITUDE, latitude, 0.0f);
  }
  void add_coastal(bool coastal) {
    fbb_.AddElement<uint8_t>(Town::VT_COASTAL, static_cast<uint8_t>(coastal), 0);
  }
  void add_population(int16_t population) {
    fbb_.AddElement<int16_t>(Town::VT_POPULATION, population, 0);
  }
  void add_townType(int16_t townType) {
    fbb_.AddElement<int16_t>(Town::VT_TOWNTYPE, townType, 0);
  }
  void add_businesses(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<Business>>> businesses) {
    fbb_.AddOffset(Town::VT_BUSINESSES, businesses);
  }
  void add_goods(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<Good>>> goods) {
    fbb_.AddOffset(Town::VT_GOODS, goods);
  }
  void add_neighbors(flatbuffers::Offset<flatbuffers::Vector<int16_t>> neighbors) {
    fbb_.AddOffset(Town::VT_NEIGHBORS, neighbors);
  }
  void add_businessCounter(int16_t businessCounter) {
    fbb_.AddElement<int16_t>(Town::VT_BUSINESSCOUNTER, businessCounter, 0);
  }
  explicit TownBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  TownBuilder &operator=(const TownBuilder &);
  flatbuffers::Offset<Town> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<Town>(end);
    return o;
  }
};

inline flatbuffers::Offset<Town> CreateTown(
    flatbuffers::FlatBufferBuilder &_fbb,
    int16_t id = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>> names = 0,
    int16_t nation = 0,
    float longitude = 0.0f,
    float latitude = 0.0f,
    bool coastal = false,
    int16_t population = 0,
    int16_t townType = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<Business>>> businesses = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<Good>>> goods = 0,
    flatbuffers::Offset<flatbuffers::Vector<int16_t>> neighbors = 0,
    int16_t businessCounter = 0) {
  TownBuilder builder_(_fbb);
  builder_.add_neighbors(neighbors);
  builder_.add_goods(goods);
  builder_.add_businesses(businesses);
  builder_.add_latitude(latitude);
  builder_.add_longitude(longitude);
  builder_.add_names(names);
  builder_.add_businessCounter(businessCounter);
  builder_.add_townType(townType);
  builder_.add_population(population);
  builder_.add_nation(nation);
  builder_.add_id(id);
  builder_.add_coastal(coastal);
  return builder_.Finish();
}

inline flatbuffers::Offset<Town> CreateTownDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    int16_t id = 0,
    const std::vector<flatbuffers::Offset<flatbuffers::String>> *names = nullptr,
    int16_t nation = 0,
    float longitude = 0.0f,
    float latitude = 0.0f,
    bool coastal = false,
    int16_t population = 0,
    int16_t townType = 0,
    const std::vector<flatbuffers::Offset<Business>> *businesses = nullptr,
    const std::vector<flatbuffers::Offset<Good>> *goods = nullptr,
    const std::vector<int16_t> *neighbors = nullptr,
    int16_t businessCounter = 0) {
  return Save::CreateTown(
      _fbb,
      id,
      names ? _fbb.CreateVector<flatbuffers::Offset<flatbuffers::String>>(*names) : 0,
      nation,
      longitude,
      latitude,
      coastal,
      population,
      townType,
      businesses ? _fbb.CreateVector<flatbuffers::Offset<Business>>(*businesses) : 0,
      goods ? _fbb.CreateVector<flatbuffers::Offset<Good>>(*goods) : 0,
      neighbors ? _fbb.CreateVector<int16_t>(*neighbors) : 0,
      businessCounter);
}

struct Game FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum {
    VT_TOWNS = 4,
    VT_TRAVELERS = 6
  };
  const flatbuffers::Vector<flatbuffers::Offset<Town>> *towns() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<Town>> *>(VT_TOWNS);
  }
  const flatbuffers::Vector<flatbuffers::Offset<Traveler>> *travelers() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<Traveler>> *>(VT_TRAVELERS);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_TOWNS) &&
           verifier.VerifyVector(towns()) &&
           verifier.VerifyVectorOfTables(towns()) &&
           VerifyOffset(verifier, VT_TRAVELERS) &&
           verifier.VerifyVector(travelers()) &&
           verifier.VerifyVectorOfTables(travelers()) &&
           verifier.EndTable();
  }
};

struct GameBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_towns(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<Town>>> towns) {
    fbb_.AddOffset(Game::VT_TOWNS, towns);
  }
  void add_travelers(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<Traveler>>> travelers) {
    fbb_.AddOffset(Game::VT_TRAVELERS, travelers);
  }
  explicit GameBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  GameBuilder &operator=(const GameBuilder &);
  flatbuffers::Offset<Game> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<Game>(end);
    return o;
  }
};

inline flatbuffers::Offset<Game> CreateGame(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<Town>>> towns = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<Traveler>>> travelers = 0) {
  GameBuilder builder_(_fbb);
  builder_.add_travelers(travelers);
  builder_.add_towns(towns);
  return builder_.Finish();
}

inline flatbuffers::Offset<Game> CreateGameDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    const std::vector<flatbuffers::Offset<Town>> *towns = nullptr,
    const std::vector<flatbuffers::Offset<Traveler>> *travelers = nullptr) {
  return Save::CreateGame(
      _fbb,
      towns ? _fbb.CreateVector<flatbuffers::Offset<Town>>(*towns) : 0,
      travelers ? _fbb.CreateVector<flatbuffers::Offset<Traveler>>(*travelers) : 0);
}

inline const Save::Game *GetGame(const void *buf) {
  return flatbuffers::GetRoot<Save::Game>(buf);
}

inline const Save::Game *GetSizePrefixedGame(const void *buf) {
  return flatbuffers::GetSizePrefixedRoot<Save::Game>(buf);
}

inline bool VerifyGameBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifyBuffer<Save::Game>(nullptr);
}

inline bool VerifySizePrefixedGameBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifySizePrefixedBuffer<Save::Game>(nullptr);
}

inline void FinishGameBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<Save::Game> root) {
  fbb.Finish(root);
}

inline void FinishSizePrefixedGameBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<Save::Game> root) {
  fbb.FinishSizePrefixed(root);
}

}  // namespace Save

#endif  // FLATBUFFERS_GENERATED_SAVE_SAVE_H_
