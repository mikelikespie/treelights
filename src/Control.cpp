//
// Created by Michael Lewis on 8/18/15.
//

#include "Control.h"
template <typename ValueType>
void ValueControl<ValueType>::tick(const Clock &clock, float inputValue) {
    auto newValue = computeNextValue(clock, inputValue);
    _didChange = _value != newValue;
    if (_didChange) {
        _value = newValue;
    }
}


template <typename ValueType>
void ValueControl<ValueType>::tick(const Clock &clock) {
    auto newValue = computeNextValue(clock);
    _didChange = _value != newValue;
    if (_didChange) {
        _value = newValue;
    }
}



/// Defaults to just return the default value
template <typename ValueType>
ValueType ValueControl<ValueType>::computeNextValue(const Clock &clock) {
    return value();
}

bool BooleanValueControl::computeNextValue(const Clock &clock, float inputValue) {
    return inputValue > 0.5;
}


float IdentityValueControl::computeNextValue(const Clock &clock, float inputValue) {
    return inputValue;
}

template <typename ValueType>
ValueType LinearlyInterpolatedValueControl<ValueType>::computeNextValue(const Clock &clock, float inputValue) {
    return inputValue * (_maxVal - _minVal) + _minVal;
}

