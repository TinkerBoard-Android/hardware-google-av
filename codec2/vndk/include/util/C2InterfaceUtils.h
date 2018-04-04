/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef C2UTILS_INTERFACE_UTILS_H_
#define C2UTILS_INTERFACE_UTILS_H_

#include <C2Param.h>
#include <C2Work.h>

#include <cmath>
#include <limits>
#include <type_traits>

/**
 * Helper class to map underlying types to C2Value types as well as to print field values. This is
 * generally the same as simply the underlying type except for characters (STRING) and bytes (BLOB).
 */
template<typename T>
struct C2_HIDE _C2FieldValueHelper {
    typedef T ValueType;
    inline static std::ostream& put(std::ostream &os, const C2Value::Primitive &p) {
        return os << p.ref<T>();
    }
};

template<>
struct C2_HIDE _C2FieldValueHelper<uint8_t> {
    typedef uint32_t ValueType;
    static std::ostream& put(std::ostream &os, const C2Value::Primitive &p);
};

template<>
struct C2_HIDE _C2FieldValueHelper<char> {
    typedef int32_t ValueType;
    static std::ostream& put(std::ostream &os, const C2Value::Primitive &p);
};

/**
 * Supported value range utility for a field of a given type.
 *
 * This mimics C2FieldSupportedValue for RANGE type.
 */
template<typename T>
class C2SupportedRange {
    typedef typename _C2FieldValueHelper<T>::ValueType ValueType;

//private:
    constexpr static T MIN_VALUE = std::numeric_limits<T>::min();
    constexpr static T MAX_VALUE = std::numeric_limits<T>::max();
    constexpr static T MIN_STEP = std::is_floating_point<T>::value ? 0 : 1;

public:
    /**
     * Constructs an empty range with no supported values.
     *
     * \note This is a specializated supported range representation that is only used for
     * this object - it is equivalent to the EMPTY type in C2FieldSupportedValues.
     */
    inline static constexpr C2SupportedRange<T> None() {
        return C2SupportedRange(MAX_VALUE, MIN_VALUE);
    }

    /**
     * Constructs a range with all values supported.
     */
    inline static constexpr C2SupportedRange<T> Any() {
        return C2SupportedRange(MIN_VALUE, MAX_VALUE);
    }

    /**
     * Constructs a range with a single supported value.
     *
     * \param value the sole supported value
     */
    inline static constexpr C2SupportedRange<T> EqualTo(T value) {
        return C2SupportedRange(value, value);
    }

    /**
     * Constructs a range with supported values greater than a given value.
     *
     * \param value the given value
     */
    inline static C2SupportedRange<T> GreaterThan(T value) {
        return (value == MAX_VALUE ? None() :
                std::is_floating_point<T>::value ?
                        C2SupportedRange(std::nextafter(value, MAX_VALUE), MAX_VALUE) :
                        C2SupportedRange(value + MIN_STEP, MAX_VALUE));
    }

    /**
     * Constructs a range with supported values greater than or equal to a given value.
     *
     * \param value the given value
     */
    inline static constexpr C2SupportedRange<T> GreaterThanOrEqualTo(T value) {
        return C2SupportedRange(value, MAX_VALUE);
    }

    /**
     * Constructs a range with supported values greater than or equal to (aka not less than) a given
     * value.
     *
     * \param value the given value
     */
    inline static constexpr C2SupportedRange<T> NotLessThan(T value) {
        return GreaterThanOrEqualTo(value);
    }

    /**
     * Constructs a range with supported values less than or equal to a given value.
     *
     * \param value the given value
     */
    inline static constexpr C2SupportedRange<T> LessThanOrEqualTo(T value) {
        return C2SupportedRange(MIN_VALUE, value);
    }

    /**
     * Constructs a range with supported values less than or equal to (aka not greater than) a given
     * value.
     *
     * \param value the given value
     */
    inline static constexpr C2SupportedRange<T> NotGreaterThan(T value) {
        return LessThanOrEqualTo(value);
    }

    /**
     * Constructs a range with supported values less than a given value.
     *
     * \param value the given value
     */
    inline static C2SupportedRange<T> LessThan(T value) {
        return (value == MIN_VALUE ? None() :
                std::is_floating_point<T>::value ?
                        C2SupportedRange(MIN_VALUE, std::nextafter(value, MIN_VALUE)) :
                        C2SupportedRange(MIN_VALUE, value - MIN_STEP));
    }

    /**
     * Constructs a continuous or arithmetic range between two values.
     *
     * \param min the lower value
     * \param max the higher value (if this is lower than |min| the range will be empty)
     * \param step the step of the arithmetic range. (If this is 0 for floating point types or 1 for
     *        integer types, the constructed range is continuous)
     */
    inline static constexpr
    C2SupportedRange<T> InRange(T min, T max, T step = MIN_STEP) {
        return C2SupportedRange(min, max, step);
    }

    /**
     * Constructs a range over a geometric series between two values.
     *
     * \param min the lower bound of the range. This value is always part of the constructed range
     *        as long as it is not greater than |max|.
     * \param max the upper bound of the range. This value is only part of the constructed
     *        range if it is part of the geometric series.
     * \param num the numerator of the geometric series.
     * \param denom the denominator of the geometric series.
     */
    inline static constexpr
    C2SupportedRange<T> InSeries(T min, T max, T num, T denom) {
        return C2SupportedRange(min, max, 0, num, denom);
    }

    /**
     * Constructs a range over a multiply-accumulate series between two values.
     *
     * \param min the lower bound of the range. This value is always part of the constructed range
     *        as long as it is not greater than |max|.
     * \param max the upper bound of the range. This value is only part of the constructed
     *        range if it is part of the series.
     * \param step the accumulator of the multiply-accumulate series
     * \param num the numerator of the multiply-accumulate series.
     * \param denom the denominator of the multiply-accumulate series.
     */
    inline static constexpr
    C2SupportedRange<T> InMacSeries(T min, T max, T step, T num, T denom) {
        return C2SupportedRange(min, max, step, num, denom);
    }

    /**
     * Constructs a range from a generic C2FieldSupportedValues object. This will be an empty
     * range if the supported values are not of RANGE type.
     *
     * \param values the supported values object
     */
    C2SupportedRange(const C2FieldSupportedValues &values);

    /**
     * Returns whether this range is empty.
     */
    inline constexpr bool isEmpty() const {
        return _mMin > _mMax;
    }

    /**
     * Returns whether this range is valid.
     *
     * Ranges are valid if they are continuous or monotonic.
     */
    inline constexpr bool isValid() const {
        // TODO: handle overflow or negative series
        return _mDenom > 0 && _mNum >= _mDenom && _mMin * (_mDenom - _mNum) < _mStep * _mDenom;
    }

    /**
     * Returns whether a value is part of this range.
     *
     * \param value the value to check.
     */
    bool contains(T value) const;

    /**
     * Returns a new range that is the intersection of this range and another, if it is
     * representable as a range object.
     *
     * \param limit the other range
     */
    C2SupportedRange<T> limitedTo(const C2SupportedRange<T> &limit) const;

    /**
     * Converts this object to a C2FieldSupportedValues object.
     */
    inline operator C2FieldSupportedValues() const {
        return C2FieldSupportedValues(_mMin, _mMax, _mStep, _mNum, _mDenom);
    }

    /**
     * Returns the lower bound and starting point of this range.
     */
    inline C2_HIDE constexpr T min() const   { return _mMin;   }

    /**
     * Returns the upper bound of this range.
     */
    inline C2_HIDE constexpr T max() const   { return _mMax;   }

    /**
     * Returns the step of this range.
     */
    inline C2_HIDE constexpr T step() const  { return _mStep;  }

    /**
     * Returns the numerator of this range.
     */
    inline C2_HIDE constexpr T num() const   { return _mNum;   }

    /**
     * Returns the denominator of this range.
     */
    inline C2_HIDE constexpr T denom() const { return _mDenom; }

private:
    /**
     * Returns whether x[i...] is all values between _mMin and _mMax.
     */
    inline C2_HIDE constexpr bool isSimpleRange() const {
        return _mStep == MIN_STEP && _mNum == 1 && _mDenom == 1;
    }

    /**
     * Returns whether x[i...] is defined as such:
     * x[i + 1] = x[i] + _mStep, where _mStep > 0 and _mMin <= x[i] <= _mMax
     */
    inline C2_HIDE constexpr bool isArithmeticSeries() const {
        return _mStep > MIN_STEP && _mNum == 1 && _mDenom == 1;
    }

    /**
     * Returns whether x[i...] is defined as such:
     * x[i] = x[0] * (_mNum / _mDenom) ^ i (with rounding), where _mNum > _mDenom > 0 and x[0] > 0
     */
    inline C2_HIDE constexpr bool isGeometricSeries() const {
        return _mMin > 0 && _mStep == 0 && _mNum > _mDenom && _mDenom > 0;
    }

    /**
     * Returns whether x[i...] is defined as such:
     * x[i + 1] = x[i] * _mNum / _mDenom + _mStep (with rounding), while x[i + 1] > x[i], where
     * _mStep != 0, _mDenom > 0 and _mNum > 0
     */
    inline C2_HIDE constexpr bool isMacSeries() const {
        return _mStep != 0 && _mNum > 0 && _mDenom > 0;
    }

    /**
     * Constructs an arithmetic or continuous range.
     *
     * \param min the lower value
     * \param max the higher value (if this is lower than |min| the range will be empty)
     * \param step the step of the arithmetic range. (If this is 0 for floating point types or 1 for
     *        integer types, the constructed range is continuous)
     */
    constexpr C2_HIDE C2SupportedRange(T min, T max, T step = T(std::is_floating_point<T>::value ? 0 : 1))
        : _mMin(min), _mMax(max), _mStep(step), _mNum(1), _mDenom(1) { }

    /**
     * Constructs a range over a geomertic sor multiply-accumulate series.
     *
     * \param min the lower bound of the range. This value is always part of the constructed range
     *        as long as it is not greater than |max|.
     * \param max the upper bound of the range. This value is only part of the constructed
     *        range if it is part of the geometric series.
     * \param step the accumulator of the multiply-accumulate series. This is 0 for a pure geometric
     *        series
     * \param num the numerator of the geometric series.
     * \param denom the denominator of the geometric series.
     */
    constexpr C2_HIDE C2SupportedRange(T min, T max, T step, T num, T den)
        : _mMin(min), _mMax(max), _mStep(step), _mNum(num), _mDenom(den) { }

    T _mMin; ///< lower bound and starting point
    T _mMax; ///< upper bound
    T _mStep; ///< step of an arithmetic series (0 if continuous floating point range)
    T _mNum; ///< numerator of a geometric series
    T _mDenom; ///< denominator of a geometric series
};

/**
 * Ordered supported value set for a field of a given type.
 */
template<typename T>
class C2SupportedValueSet {
    typedef typename _C2FieldValueHelper<T>::ValueType ValueType;

public:
    /**
     * Constructs an empty value set.
     *
     * \note This is a specializated supported range representation that is only used for
     * this object - it is equivalent to the EMPTY type in C2FieldSupportedValues.
     */
    static inline C2SupportedValueSet<T> None() {
        return C2SupportedValueSet({ });
    }

    /**
     * Constructs a value set of given values.
     *
     * \param values the ordered set of values as an initializer list.
     */
    static inline C2SupportedValueSet<T> OneOf(const std::initializer_list<T> values) {
        return C2SupportedValueSet(values);
    }

    /**
     * Constructs a value set of given values.
     *
     * \param values the ordered set of values.
     */
    static inline C2SupportedValueSet<T> OneOf(const std::vector<T> &values) {
        return C2SupportedValueSet(values);
    }

    /**
     * Constructs a value set from a generic C2FieldSupportedValues object. This will be an empty
     * set if the supported values are not of VALUES type.
     *
     * \param values the supported values object
     */
    C2SupportedValueSet<T>(const C2FieldSupportedValues &values) {
        if (values.type == C2FieldSupportedValues::VALUES) {
            _mValues.insert(_mValues.end(), values.values.begin(), values.values.end());
        }
    }

    /**
     * Returns whether this range is empty.
     */
    constexpr bool isEmpty() const {
        return _mValues.empty();
    }

    /**
     * Returns whether a value is part of this set.
     *
     * \param value the value to check.
     */
    bool contains(T value) const;

    /**
     * Returns a new value set that is the intersection of this set and another.
     *
     * \param limit the other value set
     */
    C2SupportedValueSet<T> limitedTo(const C2SupportedValueSet<T> &limit) const;

    /**
     * Returns a new value set that is the intersection of this set and a value range.
     *
     * \param limit the other range
     */
    C2SupportedValueSet<T> limitedTo(const C2SupportedRange<T> &limit) const;

    /**
     * Converts this object to a C2FieldSupportedValues object.
     */
    operator C2FieldSupportedValues() const {
        return C2FieldSupportedValues(false /* flags */, _mValues);
    }

    /**
     * Returns the ordered set of values of this object.
     */
    const std::vector<T> values() const;

    /**
     * Clears this supported value set.
     */
    inline void clear() {
        _mValues.clear();
    }

private:
    /**
     * Constructs a value set from a set of C2Value::Primitive values.
     *
     * \param values the set
     */
    C2SupportedValueSet(std::vector<C2Value::Primitive> &&values)
        : _mValues(values) {
    }

    /**
     * Constructs a value set from a set of values.
     *
     * \param values the set
     */
    C2SupportedValueSet(const std::vector<T> &values) {
        for (T elem : values) {
            _mValues.emplace_back(elem);
        }
    }

    /**
     * Constructs a value set from a set of initializer list values
     *
     * \param values the set
     */
    C2SupportedValueSet(const std::initializer_list<T> values) {
        for (T elem : values) {
            _mValues.emplace_back(elem);
        }
    }

    std::vector<C2Value::Primitive> _mValues; ///< the supported set of values
};

/**
 * Helper class to handle C2FieldSupporteValues object for fields of various types.
 */
template<typename T>
class C2FieldSupportedValuesHelper;

// templated operator must be predeclared for friend declaration
template<typename T>
std::ostream& operator<<(std::ostream& os, const C2FieldSupportedValuesHelper<T> &i);

template<typename T>
class C2FieldSupportedValuesHelper {
public:
    /**
     * Creates a helper for a specific type from a generic C2FieldSupportedValues struct.
     */
    C2FieldSupportedValuesHelper(const C2FieldSupportedValues &values);

    // TRICKY: needed for std::unique_ptr<Impl> declaration
    ~C2FieldSupportedValuesHelper();

    // support copy constructor/operator
    C2FieldSupportedValuesHelper(const C2FieldSupportedValuesHelper &);
    C2FieldSupportedValuesHelper& operator=(const C2FieldSupportedValuesHelper &);

    bool supports(T value) const;

private:
    // use pimpl as implementation may change in the future
    struct Impl;
    std::unique_ptr<Impl> _mImpl;

    friend std::ostream& operator<< <T>(std::ostream& os, const C2FieldSupportedValuesHelper<T> &i);
    //friend std::ostream& operator<<(std::ostream& os, const C2FieldSupportedValuesHelper &i);
};

/**
 * Builder for supported values for a field of a given type.
 *
 * This builder can be used to successively restrict the supported values for a field. Upon
 * creation, there are no supported values specified - which for this builder means that all
 * values are supported.
 */
template<typename T>
class C2ParamFieldValuesBuilder {
public:
    /**
     * Creates a builder with no defined values - but implicitly any value allowed.
     */
    C2ParamFieldValuesBuilder(const C2ParamField &field);

    /**
     * Get C2ParamFieldValues from this builder.
     */
    operator C2ParamFieldValues() const;

    /**
     * Define the supported values as the currently supported values of this builder.
     */
    C2ParamFieldValuesBuilder<T> &any();

    /**
     * Restrict (and thus define) the supported values to none.
     *
     * \note This really should not be used from the builder as all params must have supported
     *       values, but is here in case this is really the case.
     */
    C2ParamFieldValuesBuilder<T> &none();

    /**
     * Restrict (and thus define) the supported values to |value| alone.
     */
    C2ParamFieldValuesBuilder<T> &equalTo(T value);

    /**
     * Restrict (and thus define) the supported values to values greater than |value|.
     */
    inline C2ParamFieldValuesBuilder<T> &greaterThan(T value) {
        return limitTo(C2SupportedRange<T>::GreaterThan(value));
    }

    /**
     * Restrict (and thus define) the supported values to values greater than or equal to |value|.
     */
    C2ParamFieldValuesBuilder<T> &greaterThanOrEqualTo(T value) {
        return limitTo(C2SupportedRange<T>::GreaterThanOrEqualTo(value));
    }

    /**
     * Restrict (and thus define) the supported values to values greater than or equal to |value|.
     */
    C2ParamFieldValuesBuilder<T> &notLessThan(T value) {
        return limitTo(C2SupportedRange<T>::NotLessThan(value));
    }

    /**
     * Restrict (and thus define) the supported values to values less than or equal to |value|.
     */
    C2ParamFieldValuesBuilder<T> &lessThanOrEqualTo(T value) {
        return limitTo(C2SupportedRange<T>::LessThanOrEqualTo(value));
    }

    /**
     * Restrict (and thus define) the supported values to values less than or equal to |value|.
     */
    C2ParamFieldValuesBuilder<T> &notGreaterThan(T value) {
        return limitTo(C2SupportedRange<T>::NotGreaterThan(value));
    }

    /**
     * Restrict (and thus define) the supported values to values less than |value|.
     */
    C2ParamFieldValuesBuilder<T> &lessThan(T value) {
        return limitTo(C2SupportedRange<T>::LessThan(value));
    }

    /**
     * Restrict (and thus define) the supported values to values in the range of [ |min|, |max| ]
     * with optional |step|.
     */
    C2ParamFieldValuesBuilder<T> &inRange(
            T min, T max, T step = std::is_floating_point<T>::value ? T(0) : T(1))  {
        return limitTo(C2SupportedRange<T>::InRange(min, max, step));
    }

    /**
     * Restrict (and thus define) the supported values to values in the geometric series starting
     * from |min| with factor |num| / |denom|, not greater than |max|.
     */
    C2ParamFieldValuesBuilder<T> &inSeries(T min, T max, T num, T denom) {
        return limitTo(C2SupportedRange<T>::InSeries(min, max, num, denom));
    }

    /**
     * Restrict (and thus define) the supported values to values in the multiply-accumulate series
     * starting from |min| with factor |num| / |denom| and |step|, not greater than |max|.
     */
    C2ParamFieldValuesBuilder<T> &inMacSeries(T min, T max, T step, T num, T denom) {
        return limitTo(C2SupportedRange<T>::InMacSeries(min, max, step, num, denom));
    }

    /**
     * Restrict (and thus define) the supported values to values in |values|.
     */
    C2ParamFieldValuesBuilder<T> &oneOf(const std::initializer_list<T> values) {
        return limitTo(C2SupportedValueSet<T>::OneOf(values));
    }

    /**
     * Restrict (and thus define) the supported values to values in |values|.
     */
    C2ParamFieldValuesBuilder<T> &oneOf(const std::vector<T> &values) {
        return limitTo(C2SupportedValueSet<T>::OneOf(values));
    }

    virtual ~C2ParamFieldValuesBuilder();

    // support copy constructor/operator
    C2ParamFieldValuesBuilder(const C2ParamFieldValuesBuilder &);
    C2ParamFieldValuesBuilder& operator=(const C2ParamFieldValuesBuilder &);

private:
    /**
     * Restrict (and thus define) the supported values to a value set.
     */
    C2ParamFieldValuesBuilder<T> &limitTo(const C2SupportedValueSet<T> &limit);

    /**
     * Restrict (and thus define) the supported values to a range.
     */
    C2ParamFieldValuesBuilder<T> &limitTo(const C2SupportedRange<T> &limit);

    struct Impl;
    std::unique_ptr<Impl> _mImpl;
};

/**
 * Builder for a list of setting conflicts.
 */
class C2SettingConflictsBuilder {
public:
    /**
     * Creates an empty list of setting conflicts.
     */
    C2SettingConflictsBuilder();

    /**
     * Creates a list containing a single setting conflict.
     */
    C2SettingConflictsBuilder(C2ParamFieldValues &&conflict);

    // TODO: add plus()

    /**
     * Gets the current list of conflicts (and moves them out of this builder.)
     * (this is why it is not const)
     */
    std::vector<C2ParamFieldValues> retrieveConflicts();

private:
    std::vector<C2ParamFieldValues> _mConflicts;
};

/**
 * Setting result builder for a parameter.
 */
struct C2SettingResultBuilder {
    /**
     * Creates a read-only setting result failure.
     *
     * This does not take FSV as only the current value of the field is supported.
     */
    static C2SettingResult ReadOnly(const C2ParamField &param);

    /**
     * Creates a bad-value setting result failure.
     *
     * This does not take FSV as the value is outside of the possible values. As such, there are no
     * conflicts for this case either.
     */
    static C2SettingResult BadValue(const C2ParamField &paramField);

    /**
     * Creates a conflict setting result failure.
     *
     * This takes FSV so use paramFieldValues and optional conflicts.
     */
    static C2SettingResult Conflict(
            C2ParamFieldValues &&paramFieldValues, C2SettingConflictsBuilder &conflicts);

    // TODO: retrieve results

    /**
     * Creates an informational conflict setting result failure with no explanation.
     */
    //C2SettingResultBuilder(const C2ParamField &paramField);

private:
    C2ParamField _mParamField;
    C2SettingResult _mResult;

    C2SettingResultBuilder(const C2SettingResultBuilder &) = delete;
};

/**
 * Setting results (PLURAL) builder.
 *
 * Setting results contain a failure status along with a list of failing fields or params.
 */
struct C2SettingResultsBuilder {
    C2SettingResultsBuilder(const C2SettingResultsBuilder&) = delete;
    C2SettingResultsBuilder(C2SettingResultsBuilder&&) = default;

    /** \returns (default) successful result with no details. */
    inline static C2SettingResultsBuilder Ok() {
        return C2SettingResultsBuilder(C2_OK);
    }

    /** \returns Interface is in bad state, with no further details. */
    inline static C2SettingResultsBuilder BadState() {
        return C2SettingResultsBuilder(C2_BAD_STATE);
    }

    /** \returns Interface connection timed out, with no further details. */
    inline static C2SettingResultsBuilder TimedOut() {
        return C2SettingResultsBuilder(C2_TIMED_OUT);
    }

    /** \returns Interface connection is corrupted, with no further details. */
    inline static C2SettingResultsBuilder Corrupted() {
        return C2SettingResultsBuilder(C2_CORRUPTED);
    }

    inline static C2SettingResultsBuilder NoMemory(C2Param::Index index_ __unused) {
        // TODO: try to add failure result
        return C2SettingResultsBuilder(C2_NO_MEMORY);
    }

    // TODO: this should not be a constructor
    /** Creates a builder with a single bad value setting result. */
    C2SettingResultsBuilder(C2SettingResult &&result);

    /** Combines this results with other results. */
    C2SettingResultsBuilder plus(C2SettingResultsBuilder&& results);

    /** Retrieve (get and move out) failures and return the failure status. */
    c2_status_t retrieveFailures(std::vector<std::unique_ptr<C2SettingResult>>* const failures);

private:
    /** Setting results based on a single status. This is used when actual setting could not be
     *  attempted to get a single C2SettingResult, or when a setting succeeded without
     *  'complaints'. */
    C2SettingResultsBuilder(c2_status_t status);
        // status must be one of OK, BAD_STATE, TIMED_OUT, CORRUPTED or NO_MEMORY
        // mainly: BLOCKING, BAD_INDEX, BAD_VALUE and NO_MEMORY requires a setting attempt, but
        // NO_MEMORY may not allow us to create a results structure.

    /**
     * One of OK, BAD_INDEX, BAD_VALUE, BAD_STATE, NO_MEMORY, TIMED_OUT, BLOCKING or CORRUPTED.
     */
    c2_status_t _mStatus __unused;

    /**
     * Vector of individual setting result details.
     */
    std::vector<std::unique_ptr<C2SettingResult>> _mResults;
};

#include <util/C2Debug-interface.h>

#endif  // C2UTILS_INTERFACE_UTILS_H_
