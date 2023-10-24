/*
 * CairoLabel for the DISTRHO Plugin Framework (DPF)
 *
 * SPDX-License-Identifier:  0BSD 
 *
 * Copyright (C) 2023 brummer <brummer@web.de>
 */

#pragma once

#ifndef CAIROLABEL_H
#define CAIROLABEL_H

#include "CairoColourTheme.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

class CairoLabel : public CairoSubWidget
{
public:

    explicit CairoLabel(SubWidget* const parent, CairoColourTheme &theme_, const char* lab)
        : CairoSubWidget(parent),
          theme(theme_),
          label(lab) {}

    explicit CairoLabel(TopLevelWidget* const parent, CairoColourTheme &theme_, const char* lab)
        : CairoSubWidget(parent),
          theme(theme_),
          label(lab) {}

protected:

    void onCairoDisplay(const CairoGraphicsContext& context) override
    {
        cairo_t* const cr = context.handle;

        const Size<uint> sz = getSize();
        const int width = sz.getWidth();
        const int height = sz.getHeight();

        cairo_push_group (cr);

        theme.setCairoColour(cr, theme.idColourForgroundNormal);
        cairo_text_extents_t extents;
        cairo_set_font_size (cr, height * 0.45);
        cairo_text_extents(cr,label , &extents);
        cairo_move_to (cr, (width * 0.5)-(extents.width * 0.5),  height * 0.5 +extents.height * 0.5);
        cairo_show_text(cr, label);
        cairo_new_path (cr);

        cairo_pop_group_to_source (cr);
        cairo_paint (cr);
    }

private:
    CairoColourTheme &theme;
    const char* label;
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CairoLabel)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif
