//
// Created by Michael Lewis on 8/17/15.
//

#ifndef TREELIGHTS_EXAMPLESEQUENCE_H
#define TREELIGHTS_EXAMPLESEQUENCE_H


#include "SequenceBase.h"
#include "Control.h"
#include <vector>

/// Just copy the next 2 lines and replace the words "ExampleSequence"

class ExampleSequence : public SequenceBase<ExampleSequence> {
public:
    ExampleSequence(int stripCount, int stripLength, const Clock &clock, ARGB color) : SequenceBase<ExampleSequence>::SequenceBase(stripCount, stripLength, clock), _color(color) {
    }
    
    inline ARGB colorForPixel(int strip, int pixel, const Context &context) {
        return _color;
    }
    
private:
    const ARGB _color;
};

class HSVSequence : public SequenceBase<HSVSequence> {
public:
    HSVSequence(int stripCount,
                int stripLength,
                const Clock &clock)
    : SequenceBase<HSVSequence>::SequenceBase(stripCount, stripLength, clock) {
    }
    
    inline ARGB colorForPixel(int strip, int pixel, const Context &context) {
        return HCL { _hControl.value(), _sControl.value(), _vControl.value() };
    }
    
    virtual const std::vector<Control *> &controls() {
        return _controls;
    }
    
private:
    IdentityValueControl _hControl = IdentityValueControl();
    IdentityValueControl _sControl = IdentityValueControl();
    IdentityValueControl _vControl = IdentityValueControl();
    
    const std::vector<Control *> _controls = {&_hControl, &_sControl, &_vControl};
};

class RGBSequence : public SequenceBase<RGBSequence> {
public:
    RGBSequence(int stripCount,
                int stripLength,
                const Clock &clock)
    : SequenceBase<RGBSequence>::SequenceBase(stripCount, stripLength, clock) {
    }
    
    inline ARGB colorForPixel(int strip, int pixel, const Context &context) {
        return ARGB { 8, _rControl.value(), _gControl.value(), _bControl.value() };
    }
    
    virtual const std::vector<Control *> &controls() {
        return _controls;
    }

private:
    IdentityValueControl _rControl = IdentityValueControl();
    IdentityValueControl _gControl = IdentityValueControl();
    IdentityValueControl _bControl = IdentityValueControl();

    const std::vector<Control*> _controls = {&_rControl, &_gControl, &_bControl};
};


#endif //TREELIGHTS_EXAMPLESEQUENCE_H
