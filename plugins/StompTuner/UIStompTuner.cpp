/*
 * StompTuner audio effect based on DISTRHO Plugin Framework (DPF)
 *
 * SPDX-License-Identifier:  GPL-2.0 license 
 *
 * Copyright (C) 2023 brummer <brummer@web.de>
 */

#include "UIStompTuner.hpp"
#include "Window.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------
// Init / Deinit

UIStompTuner::UIStompTuner()
: UI(285, 400, true), theme(), fResizeHandle(this) {
    kInitialHeight = 400;
    kInitialWidth = 285;
    blocked = false;
    sizeGroup = new UiSizeGroup(kInitialWidth, kInitialHeight);
    
    theme.setIdColour(theme.idColourBackgroundActive, 0.0, 0.898, 0.647, 1.0);
    theme.setIdColour(theme.idColourBackground, 0.07, 0.106, 0.188, 1.0);
    theme.setIdColour(theme.idColourForground, 0.0, 0.898, 0.647, 0.6);
    theme.setIdColour(theme.idColourForgroundNormal, 0.424, 0.455, 0.494, 1.0);

    tunerDisplay = new CairoTunerWidget(this, theme);
    sizeGroup->addToSizeGroup(tunerDisplay, 32, 50, 220, 140);

    refFrequency = new CairoValueDisplay(this, theme, "Reference Pitch", &blocked,
                dynamic_cast<UI*>(this), PluginStompTuner::REFFREQ);
    refFrequency->setAdjustment(440.0, 432.0, 452.0, 0.1);
    refFrequency->setUiValue = [this] (const uint32_t index, float value)
                                {this->setRefFreq(index, value);};
    sizeGroup->addToSizeGroup(refFrequency, 30, 200, 220, 30);

    bypassLed = new CairoLed(this, theme);
    sizeGroup->addToSizeGroup(bypassLed, 132, 20, 20, 20);

    bypassSwitch = new CairoPushButton(this, theme, &blocked, bypassLed,
                dynamic_cast<UI*>(this), "StompTuner", PluginStompTuner::dpf_bypass);
    sizeGroup->addToSizeGroup(bypassSwitch, 30, 240, 225, 130);
    
    if (isResizable()) fResizeHandle.hide();
}

UIStompTuner::~UIStompTuner() {

}

// -----------------------------------------------------------------------
// DSP/Plugin callbacks

/**
  A parameter has changed on the plugin side.
  This is called by the host to inform the UI about parameter changes.
*/
void UIStompTuner::parameterChanged(uint32_t index, float value) {
    // fprintf(stderr, "index %i, value %f\n", index, value);
    switch (index) {
         case PluginStompTuner::dpf_bypass:
            bypassSwitch->setValue(value);
            bypassLed->setValue(value);
            break;
         case PluginStompTuner::FREQ:
            tunerDisplay->setFrequency(value);
            break;
         case PluginStompTuner::REFFREQ:
            tunerDisplay->setRefFreq(value);
            break;
   }
}

void UIStompTuner::setRefFreq(uint32_t, float value) {
    tunerDisplay->setRefFreq(value);
}

/**
  Optional callback to inform the UI about a sample rate change on the plugin side.
*/
void UIStompTuner::sampleRateChanged(double newSampleRate) {
    (void) newSampleRate ;
}

// -----------------------------------------------------------------------
// Optional UI callbacks

/**
  Idle callback.
  This function is called at regular intervals.
*/
void UIStompTuner::uiIdle() {
    repaint();
}

/**
  Window reshape function, called when the parent window is resized.
*/
void UIStompTuner::uiReshape(uint width, uint height) {
    (void)width;
    (void)height;
}

// -----------------------------------------------------------------------
// Widget callbacks

/**
  A function called to draw the view contents.
*/
void UIStompTuner::onCairoDisplay(const CairoGraphicsContext& context) {
    cairo_t* const cr = context.handle;
    if (cairo_status(cr) != CAIRO_STATUS_SUCCESS) return;
    int width = getWidth();
    int height = getHeight();
    const float scaleH = sizeGroup->getScaleHFactor();
    const float scaleW = sizeGroup->getScaleWFactor();

    cairo_push_group (cr);

    theme.setCairoColour(cr, theme.idColourBackground);
    cairo_paint(cr);
    theme.boxShadow(cr, width, height, 25, 25);

    cairo_rectangle(cr, 25 * scaleW, 25 * scaleH, width - (50 * scaleW), height - (50 * scaleH));
    theme.setCairoColour(cr, theme.idColourBackgroundNormal);
    cairo_fill_preserve(cr);
    theme.setCairoColour(cr, theme.idColourBoxShadow);
    cairo_set_line_width(cr,8);
    cairo_stroke_preserve(cr);
    theme.setCairoColour(cr, theme.idColourBackground, 1.0f);
    cairo_set_line_width(cr,1);
    cairo_stroke(cr);

    cairo_pop_group_to_source (cr);
    cairo_paint (cr);
}

void UIStompTuner::onResize(const ResizeEvent& ev)
{
    UI::onResize(ev);
    sizeGroup->resizeAspectSizeGroup(ev.size.getWidth(), ev.size.getHeight());
}

UI* createUI() {
    return new UIStompTuner;
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
