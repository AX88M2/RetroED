/**
 * \file
 *
 * \author Mattia Basaglia
 *
 * \copyright Copyright (C) 2013-2019 Mattia Basaglia
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef COLOR_DIALOG_HPP
#define COLOR_DIALOG_HPP

#include "color_preview.hpp"
#include "color_wheel.hpp"

#include <QDialog>

class QAbstractButton;

namespace color_widgets
{

class ColorDialog : public QDialog
{
    Q_OBJECT
    Q_ENUMS(ButtonMode)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged DESIGNABLE true)
    Q_PROPERTY(ColorWheel::DisplayFlags wheelFlags READ wheelFlags WRITE setWheelFlags NOTIFY
                   wheelFlagsChanged)
    /**
     * \brief whether the color alpha channel can be edited.
     *
     * If alpha is disabled, the selected color's alpha will always be 255.
     */
    Q_PROPERTY(bool alphaEnabled READ alphaEnabled WRITE setAlphaEnabled NOTIFY alphaEnabledChanged)

public:
    enum ButtonMode { OkCancel, OkApplyCancel, Close };

    explicit ColorDialog(QWidget *parent = 0, Qt::WindowFlags f = (Qt::WindowFlags)0);

    ~ColorDialog();

    /**
     * Get currently selected color
     */
    QColor color() const;

    /**
     * Set the display mode for the color preview
     */
    void setPreviewDisplayMode(ColorPreview::DisplayMode mode);

    /**
     * Get the color preview diplay mode
     */
    ColorPreview::DisplayMode previewDisplayMode() const;

    bool alphaEnabled() const;

    /**
     * Select which dialog buttons to show
     *
     * There are three predefined modes:
     * OkCancel - this is useful when the dialog is modal and we just want to return a color
     * OkCancelApply - this is for non-modal dialogs
     * Close - for non-modal dialogs with direct color updates via colorChanged signal
     */
    void setButtonMode(ButtonMode mode);
    ButtonMode buttonMode() const;

    QSize sizeHint() const;

    ColorWheel::DisplayFlags wheelFlags() const;

public Q_SLOTS:

    /**
     * Change color
     */
    void setColor(const QColor &c);

    /**
     * Set the current color and show the dialog
     */
    void showColor(const QColor &oldcolor);

    void setWheelFlags(ColorWheel::DisplayFlags flags);

    /**
     * Set whether the color alpha channel can be edited.
     * If alpha is disabled, the selected color's alpha will always be 255.
     */
    void setAlphaEnabled(bool a);

Q_SIGNALS:
    /**
     * The current color was changed
     */
    void colorChanged(QColor);

    /**
     * The user selected the new color by pressing Ok/Apply
     */
    void colorSelected(QColor);

    void wheelFlagsChanged(ColorWheel::DisplayFlags flags);
    void alphaEnabledChanged(bool alphaEnabled);

private Q_SLOTS:
    /// Update all the Ui elements to match the selected color
    void setColorInternal(const QColor &color);
    /// Update from HSV sliders
    void set_hsv();
    /// Update from RGB sliders
    void set_rgb();
    /// Update from Alpha slider
    void set_alpha();

    void on_edit_hex_colorChanged(const QColor &color);
    void on_edit_hex_colorEditingFinished(const QColor &color);

    void on_buttonBox_clicked(QAbstractButton *);

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

private:
    class Private;
    Private *const p;
};

} // namespace color_widgets

#endif // COLOR_DIALOG_HPP
