/*
 * CairoValueDisplay for the DISTRHO Plugin Framework (DPF)
 *
 * SPDX-License-Identifier:  0BSD 
 *
 * Copyright (C) 2023 brummer <brummer@web.de>
 */

#pragma once

#ifndef CAIROVALUEDISPLAY_H
#define CAIROVALUEDISPLAY_H

#include <functional>

#include "CairoColourTheme.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

class CairoValueDisplay : public CairoSubWidget
{
public:

    explicit CairoValueDisplay(SubWidget* const parent, CairoColourTheme &theme_,
            const char* label_, bool *blocked_, UI *ui, const uint32_t index)
        : CairoSubWidget(parent),
          theme(theme_),
          label(label_),
          blocked(blocked_),
          setParameterValue([ui] (const uint32_t index, float value)
                                {ui->setParameterValue(index, value);}),
          port(index)
          {
            init();
          }

    explicit CairoValueDisplay(TopLevelWidget* const parent, CairoColourTheme &theme_,
            const char* label_, bool *blocked_, UI *ui, const uint32_t index)
        : CairoSubWidget(parent),
          theme(theme_),
          label(label_),
          blocked(blocked_),
          setParameterValue([ui] (const uint32_t index, float value)
                                {ui->setParameterValue(index, value);}),
          port(index)
          {
            init();
          }

    void setValue(float v)
    {
        value = v;
        //repaint(); // use uiIdle() to repaint in intervals
    }
    
    void setAdjustment(float value_, float min_value_, float max_value_, float value_step_)
    {
        value = value_;
        min_value = min_value_;
        max_value = max_value_;
        value_step = value_step_;
        stepper = getStepper();
        //repaint(); // use uiIdle() to repaint in intervals
    }

    std::function<void(const uint32_t, float) > setUiValue;

protected:

    void init()
    {
        value = 0.0f;
        min_value = 0.0f;
        max_value = 1.0f;
        value_step = 0.01f;
        posY = 0.0f;
        v = 0;
        inDrag = false;
        prelight = false;
        wx = 0;
        stepper = getStepper();
        setUiValue = setDummy;
    }

    static void setDummy(const uint32_t index, float value)
    {
        (void)index;
        (void)value;
    }

    float getStepper()
    {
        float range = std::fabs(max_value - min_value);
        float steps = range / value_step;
        return steps * 0.01f;
    }

    void onCairoDisplay(const CairoGraphicsContext& context) override
    {
        cairo_t* const cr = context.handle;
        if (cairo_status(cr) != CAIRO_STATUS_SUCCESS) return;

        /** get size for the value display **/
        const Size<uint> sz = getSize();
        const int width = sz.getWidth();
        const int height = sz.getHeight();

        /** draw the value display **/
        cairo_push_group (cr);

        /** show label on the value display**/
        theme.setCairoColour(cr, theme.idColourForgroundNormal);
        cairo_text_extents_t extents;
        cairo_set_font_size (cr, height * 0.45);

        cairo_text_extents(cr,label , &extents);
        cairo_move_to (cr, (width - extents.width) * 0.15,  (height + extents.height) * 0.5);
        cairo_show_text(cr, label);
        cairo_new_path (cr);

        /** show value on the value display**/
        if (prelight) theme.setCairoColourWithAlpha(cr, theme.idColourForground, 0.81);
        else theme.setCairoColour(cr, theme.idColourForground);
        cairo_select_font_face (cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
                                   CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size (cr, height * 0.55);
        char s[17];
        const char* format[] = {"%.1f Hz", "%.2f Hz", "%.3f Hz"};
        if (fabs(value_step)>0.99) {
            snprintf(s, 16,"%d",  (int) value);
        } else if (fabs(value_step)>0.09) {
            snprintf(s, 16, format[1-1], value);
        } else {
            snprintf(s, 16, format[2-1], value);
        }
        cairo_text_extents(cr, s, &extents);
        if (std::abs(wx - extents.width) > 1) wx = extents.width;
        cairo_move_to (cr, (width - wx) * 0.95, (height + extents.height) * 0.5);
        cairo_show_text(cr, s);
        cairo_new_path (cr);

        cairo_rectangle(cr, width * 0.6, 0, width, height);
        theme.boxShadowInset(cr, width * 0.4, height, width * 0.6, 0, true);
        cairo_pop_group_to_source (cr);
        cairo_paint (cr);
    }

    bool onKeyboard(const KeyboardEvent& event) override {
        if (event.press && prelight)
        {
            int set_value = 0;
            if (event.key == 57357) set_value = 1; // UpArrow
            else if (event.key == 57359) set_value = -1; // DownArrow
            float  v = value + (value_step * set_value);
            v = MIN(max_value, MAX(min_value, v));
            setValue(v);
            setParameterValue(port, value);
            setUiValue(port, value);
        }
        return CairoSubWidget::onKeyboard(event);
    }

    bool onMouse(const MouseEvent& event) override
    {
        if (event.press && (event.button == 1) && contains(event.pos)) // mouse button pressed
        {
            posY = event.pos.getY();
            inDrag = true;
        }
        else
        {
            inDrag = false;
        }

        return CairoSubWidget::onMouse(event);
    }

    bool onScroll(const ScrollEvent& event) override
    {
        if (!contains(event.pos))
            return CairoSubWidget::onScroll(event);

        const float set_value = (event.delta.getY() > 0.f) ? 1.f : -1.f;
        float  v1 = value + (value_step * set_value);
        v1 = MIN(max_value, MAX(min_value, v1));
        setValue(v1);
        setParameterValue(port, value);
        setUiValue(port, value);
        
        return CairoSubWidget::onScroll(event);
    }

    bool onMotion(const MotionEvent& event) override
    {
        if (inDrag && (std::abs(posY - event.pos.getY()) > 0.f))
        {
            const float set_value = (posY - event.pos.getY() > 0.f) ? 1.f : -1.f ;
            v += stepper * value_step;
            posY = event.pos.getY();
            if (v >= value_step)
            {
                v = value + (value_step * set_value);
                v = MIN(max_value, MAX(min_value, v));
                setValue(v);
                setParameterValue(port, value);
                setUiValue(port, value);
                v = 0;
            }
        }
        if (contains(event.pos)) // enter
        {
            if (!prelight && !(*blocked)) {
                prelight = true;
                (*blocked) = true;
                //repaint(); // use uiIdle() to repaint in intervals
            }
        }
        else if (prelight && !inDrag) // leave
        {
            prelight = false;
            (*blocked) = false;
            //repaint(); // use uiIdle() to repaint in intervals
        }

        return CairoSubWidget::onMotion(event);
    }

private:
    CairoColourTheme &theme;
    const char* label;
    bool *blocked;
    std::function<void(const uint32_t, float) > setParameterValue;
    float value;
    float min_value;
    float max_value;
    float value_step;
    float posY;
    float v;
    float stepper;
    bool inDrag;
    bool prelight;
    uint wx;
    const uint32_t port;
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CairoValueDisplay)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif
