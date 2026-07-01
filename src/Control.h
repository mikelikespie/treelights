//
// Created by Michael Lewis on 8/18/15.
//

#ifndef TREELIGHTS_CONTROL_H
#define TREELIGHTS_CONTROL_H


#include <math.h>
#include <stdint.h>
#include "clock.h"

/**
 * Controls take a DMX input and a clock and compute a new value. They can store state how they want.
 */
class Control {
public:
  // Called before each frame. A control should calculate its new value based on this
  virtual void tick(const Clock &clock, float inputValue) = 0;

  virtual void tick(const Clock &clock) = 0;
};

// Control that has a value
template<typename ValueType>
class ValueControl : public Control {
public:
  void tick(const Clock &clock, float inputValue) override;

  void tick(const Clock &clock) override;

  ValueControl() : _value(0) {
  };

  explicit ValueControl(ValueType defaultValue) : _value(defaultValue) {}

  inline const ValueType &value() const {
    return _value;
  }

  inline bool didChange() {
    return _didChange;
  }

protected:
  virtual ValueType computeNextValue(const Clock &clock, float inputValue) = 0;

  virtual ValueType computeNextValue(const Clock &clock);

private:
  ValueType _value;
  bool _didChange = false;
};


template<class WrappedControlType>
class BufferedControl : public ValueControl<float> {
public:
  template<typename ...WrappedControlTypeArgs>
  BufferedControl(WrappedControlTypeArgs... wrappedControlArgs) : _wrappedControl(wrappedControlArgs...) {
  }

  void tick(const Clock &clock, float inputValue) override {
    _wrappedControl.tick(clock, inputValue);
    converge();
    ValueControl<float>::tick(clock, inputValue);
  }

  // Without an input the wrapped control holds its default, so value()
  // reflects the defaults until DMX data arrives.
  void tick(const Clock &clock) override {
    _wrappedControl.tick(clock);
    converge();
    ValueControl<float>::tick(clock);
  }

protected:
  float computeNextValue(const Clock &clock, float inputValue) override {
    return _computedValue;
  }

  float computeNextValue(const Clock &clock) override {
    return _computedValue;
  }

private:
  void converge() {
    _actualValue = _wrappedControl.value();

    if (!initialized) {
      _computedValue = _actualValue;
      initialized = true;
    } else {
      // Otherwise we converge on computed value
      _computedValue = (_computedValue - _actualValue) * .9f + _actualValue;
    }
  }

  bool initialized = false;
  float _computedValue = 0;
  float _actualValue = 0;

  WrappedControlType _wrappedControl;
};

template<class WrappedControlType>
class AccumulatorControl : public ValueControl<float> {
public:
  template<typename ...WrappedControlTypeArgs>
  explicit AccumulatorControl(WrappedControlTypeArgs... wrappedControlArgs) : _wrappedControl(wrappedControlArgs...),
                                                                              _accumulatedValue(0) {

  }

  void tick(const Clock &clock, float inputValue) override {
    _wrappedControl.tick(clock, inputValue);
    _accumulatedValue += _wrappedControl.value() * clock.deltaf();
    ValueControl<float>::tick(clock, inputValue);
  }

  void tick(const Clock &clock) override {
    _wrappedControl.tick(clock);
    _accumulatedValue += _wrappedControl.value() * clock.deltaf();
    ValueControl<float>::tick(clock);
  }

  float computeNextValue(const Clock &clock, float dmxValue) override {
    return _accumulatedValue;
  }


  float computeNextValue(const Clock &clock) override {
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

  IdentityValueControl(float defaultValue) : ValueControl(defaultValue) {

  }

  IdentityValueControl() : ValueControl() {

  }

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

  LinearlyInterpolatedValueControl(ValueType minVal, ValueType maxVal, ValueType defaultValue)
          : ValueControl<ValueType>(defaultValue), _minVal(minVal), _maxVal(maxVal) {
  }

  LinearlyInterpolatedValueControl(ValueType minVal, ValueType maxVal)
          : _minVal(minVal), _maxVal(maxVal) {
  }

private:
  ValueType _minVal;
  ValueType _maxVal;
};

//typedef LinearlyInterpolatedValueControl<float> SmoothLinearControl;
typedef AccumulatorControl<LinearlyInterpolatedValueControl<float>> SmoothAccumulatorControl;
typedef BufferedControl<LinearlyInterpolatedValueControl<float>> SmoothLinearControl;

#endif //TREELIGHTS_CONTROL_H
