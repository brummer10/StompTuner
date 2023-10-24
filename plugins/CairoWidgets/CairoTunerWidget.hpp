/*
 * CairoTunerWidget for the DISTRHO Plugin Framework (DPF)
 *
 * SPDX-License-Identifier:  0BSD 
 *
 * Copyright (C) 2023 brummer <brummer@web.de>
 */

#pragma once

#ifndef CAIROTUNERWIDGET_H
#define CAIROTUNERWIDGET_H

#include "CairoColourTheme.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

class CairoTunerWidget : public CairoSubWidget
{
public:

    explicit CairoTunerWidget(SubWidget* const parent, CairoColourTheme &theme_)
        : CairoSubWidget(parent),
          theme(theme_)
          {
            init();
          }

    explicit CairoTunerWidget(TopLevelWidget* const parent, CairoColourTheme &theme_)
        : CairoSubWidget(parent),
          theme(theme_)
          {
            init();
          }

    void setFrequency(float v)
    {
        detectedFrequency = v;
        repaint();
    }

    void setRefFreq(float v)
    {
        refFreq = v;
        repaint();
    }

protected:
    void init()
    {
        detectedFrequency = 0.0f;
        detectedNote = 0;
        detectedOctave = 0;
        cent = 0.0f;
        collectCents = 0.0f;
        collectMilliCents = 0.0f;
        fw = 0;
        cw = 0;
        refFreq = 440.0;
    }

    void getNote()
    {
        if (detectedFrequency > 23.0f && detectedFrequency < 999.0f)
        {
            float scale = -0.5;

            float fdetectedNote = 12.0 * (log2f(detectedFrequency/refFreq) + 4);
            float fdetectedNoter = round(fdetectedNote);
            detectedNote = fdetectedNoter;
            detectedOctave = round((fdetectedNoter + 3)/12);
            const int octsz = sizeof(octave) / sizeof(octave[0]);
            if (detectedOctave < 0 || detectedOctave >= octsz) {
                // just safety, should not happen with current parameters
                // (pitch tracker output 23 .. 999 Hz)
                detectedOctave = octsz - 1;
            }

            scale = (fdetectedNote-detectedNote) / 4;
            cent = (scale * 10000) / 25;
            detectedNote = detectedNote % 12;
            if (detectedNote < 0) {
                detectedNote += 12;
            }
        }
        else
        {
            detectedFrequency = 0.0f;
            detectedNote = 0;
            detectedOctave = 0;
            cent = 0.0f;
        }
        
    }

    void drawStrobe(cairo_t* const cr, float di, int x, int y, int radius, float w) {
        theme.setCairoColour(cr,theme.idColourBackgroundActive);
        cairo_set_line_width(cr,6);
        int i;
        const int d = 4;
        for (i=24; i<55; i++) {
            double angle = i * 0.01 * 2 * M_PI;
            double rx = radius * sin(angle);
            double ry = radius * cos(angle);
            double length_x = x - rx;
            double length_y = y + ry;
            double radius_x = x - rx * w ;
            double radius_y = y + ry * w ;
            if ((int)di < d) {
                cairo_move_to(cr, radius_x, radius_y);
                cairo_line_to(cr,length_x,length_y);
            }
            di++;
            if (di>8.0) di = 0.0;
       }
       cairo_stroke_preserve(cr);
    }

    void onCairoDisplay(const CairoGraphicsContext& context) override
    {
        getNote();
        cairo_t* const cr = context.handle;

        const Size<uint> sz = getSize();
        const int width = sz.getWidth();
        const int height = sz.getHeight();

        cairo_push_group (cr);

        theme.setCairoColour(cr,theme.idColourFrame);
        cairo_rectangle(cr,0, 0, width, height);
        cairo_set_line_width(cr,2);
        cairo_stroke(cr);

        cairo_text_extents_t extents;
        cairo_select_font_face (cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
                               CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size (cr, height/8.2);
        char s[64];
        snprintf(s, 63, "%.2f Hz", detectedFrequency);
        cairo_text_extents(cr,s , &extents);
        if (std::abs(fw - extents.width) > 1) fw = extents.width;
        cairo_move_to (cr, width * 0.45 - fw ,  height * 0.15 + extents.height);
        theme.setCairoColour(cr, theme.idColourForground);
        cairo_show_text(cr, s);
        snprintf(s, 63, "%.2f C", cent);
        cairo_text_extents(cr,s , &extents);
        if (std::abs(cw - extents.width) > 1) cw = extents.width;
        cairo_move_to (cr, width * 0.4 - cw ,  height * 0.35 + extents.height);
        cairo_show_text(cr, s);
        theme.setCairoColour(cr,theme.idColourBackgroundActive);
        cairo_set_font_size (cr, height/3.2);
        cairo_text_extents(cr, note_sharp[detectedNote], &extents);
        cairo_move_to (cr, width * 0.6 ,  height * 0.6 + extents.height);
        if (detectedFrequency > 23.0f && detectedFrequency < 999.0f) {
            cairo_show_text(cr, note_sharp[detectedNote]);
            cairo_set_font_size (cr, height/5.3);
            cairo_show_text(cr, octave[detectedOctave]);
        } else {
            cairo_move_to (cr, width * 0.705 ,  height * 0.6 + extents.height);
            cairo_show_text(cr, "#");
        }
        cairo_new_path (cr);
        float sCent = (cent * 0.016);
        if (std::fabs(cent) >= 1.0) {
            collectCents += sCent;
            if (collectCents>8.0) collectCents = 0.0;
            else if (collectCents<0.0) collectCents = 8.0;
        }
        drawStrobe (cr, collectCents, width*0.9, height, height/1.1, 0.9);

        float sMilliCent = (cent * 0.16);
        if (std::fabs(cent) >= 0.1) {
            collectMilliCents += sMilliCent;
            if (collectMilliCents>8.0) collectMilliCents = 0.0;
            else if (collectMilliCents<0.0) collectMilliCents = 8.0;
        }
        drawStrobe (cr, collectMilliCents, width*0.9, height, height/1.25, 0.95);

        theme.boxShadowInset(cr, width, height);

        cairo_pop_group_to_source (cr);
        cairo_paint (cr);
    }

private:
    CairoColourTheme &theme;
    float detectedFrequency;
    int detectedNote;
    int detectedOctave;
    float cent;
    float collectCents;
    float collectMilliCents;
    float refFreq;
    uint fw;
    uint cw;
    static constexpr const char *note_sharp[] = {"A","A#","B","C","C#","D","D#","E","F","F#","G","G#"};
    static constexpr const char *octave[] = {"0","1","2","3","4","5"," "};
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CairoTunerWidget)
};

constexpr const char *CairoTunerWidget::note_sharp[];
constexpr const char *CairoTunerWidget::octave[];

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif
