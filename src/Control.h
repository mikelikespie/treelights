//
// Created by Michael Lewis on 8/18/15.
//

#ifndef TREELIGHTS_CONTROL_H
#define TREELIGHTS_CONTROL_H


#include <math.h>
#include <stdint.h>
#include "clock.h"
#include <functional>

/**
 * Controls take a DMX input and a clock and compute a new value. They can store state how they want.
 */
class Control {
public:
    // Called before each frame. A control should calculate its new value based on this
    virtual void tick(const Clock &clock, float inputValue) = 0;
};

// Control that has a value
template<typename ValueType>
class ValueControl : public Control {
public:
    virtual void tick(const Clock &clock, float inputValue);

    inline const ValueType &value() const {
        return _value;
    }
    
    inline bool didChange() {
        return _didChange;
    }

protected:
    virtual ValueType computeNextValue(const Clock &clock, float inputValue) = 0;

private:
    ValueType _value;
    bool _didChange = false;
};


template <class WrappedControlType>
class BufferedControl : public ValueControl<float> {
    public:
    template <typename ...WrappedControlTypeArgs>
    BufferedControl(WrappedControlTypeArgs... wrappedControlArgs) : _wrappedControl(wrappedControlArgs...) {

    }
    
    virtual void tick(const Clock &clock, float inputValue) {
        _wrappedControl.tick(clock, inputValue);
        _actualValue = _wrappedControl.value();
        
        if (!initialized) {
            _computedValue = _actualValue;
            initialized = true;
        } else {
            // Otherwise we converge on computed value
            _computedValue = (_computedValue - _actualValue) * .75f + _actualValue;
        }
        ValueControl<float>::tick(clock, inputValue);
    }
    
    virtual float computeNextValue(const Clock &clock, float inputValue) {
        return _computedValue;
    }
    
private:
    
    bool initialized = false;
    float _computedValue;
    float _actualValue;
    
    WrappedControlType _wrappedControl;
};

template <class WrappedControlType>
class AccumulatorControl : public ValueControl<float> {
public:
    template <typename ...WrappedControlTypeArgs>
    AccumulatorControl(WrappedControlTypeArgs... wrappedControlArgs) : _wrappedControl(wrappedControlArgs...) {
        
    }
    
    virtual void tick(const Clock &clock, float inputValue) {
        _wrappedControl.tick(clock, inputValue);
        _accumulatedValue += _wrappedControl.value() * clock.deltaf();
        ValueControl<float>::tick(clock, inputValue);
    }
    
    virtual float computeNextValue(const Clock &clock, float dmxValue) {
        return _accumulatedValue;
    }
    
    // Truncates the value so we don't get floating point issues
    void truncate(float frequency) {
         _accumulatedValue = fmodf(_accumulatedValue, frequency);
        if (_accumulatedValue < 0) {
            _accumulatedValue += frequency;
        }
    }
    
private:
    
    bool initialized = false;
    
    float _accumulatedValue;
    
    WrappedControlType _wrappedControl;
};


/**
 * Takes <127 = 0,
 */
class BooleanValueControl : public ValueControl<bool> {
protected:
    virtual bool computeNextValue(const Clock &clock, float inputValue);
};


// Just a passthrough
class IdentityValueControl : public ValueControl<float> {
public:

protected:
    
    virtual float computeNextValue(const Clock &clock, float dmxValue);
};


// Just a passthrough
template<typename ValueType>
class LinearlyInterpolatedValueControl : public ValueControl<ValueType> {
protected:
    virtual ValueType computeNextValue(const Clock &clock, float inputValue);

public:
    LinearlyInterpolatedValueControl() = delete;
    LinearlyInterpolatedValueControl(ValueType minVal, ValueType maxVal) : _minVal(minVal), _maxVal(maxVal) { }

private:
    ValueType _minVal;
    ValueType _maxVal;
};

template class LinearlyInterpolatedValueControl<int>;
template class LinearlyInterpolatedValueControl<float>;

#endif //TREELIGHTS_CONTROL_H
