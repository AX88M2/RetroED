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
#include "color_utils.hpp"

namespace color_widgets
{
namespace detail
{

QColor color_from_lch(qreal hue, qreal chroma, qreal luma, qreal alpha)
{
    qreal h1 = hue * 6;
    qreal x  = chroma * (1 - qAbs(std::fmod(h1, 2) - 1));
    QColor col;
    if (h1 >= 0 && h1 < 1)
        col = QColor::fromRgbF(chroma, x, 0);
    else if (h1 < 2)
        col = QColor::fromRgbF(x, chroma, 0);
    else if (h1 < 3)
        col = QColor::fromRgbF(0, chroma, x);
    else if (h1 < 4)
        col = QColor::fromRgbF(0, x, chroma);
    else if (h1 < 5)
        col = QColor::fromRgbF(x, 0, chroma);
    else if (h1 < 6)
        col = QColor::fromRgbF(chroma, 0, x);

    qreal m = luma - color_lumaF(col);

    return QColor::fromRgbF(qBound(0.0, col.redF() + m, 1.0), qBound(0.0, col.greenF() + m, 1.0),
                            qBound(0.0, col.blueF() + m, 1.0), alpha);
}

QColor color_from_hsl(qreal hue, qreal sat, qreal lig, qreal alpha)
{
    qreal chroma = (1 - qAbs(2 * lig - 1)) * sat;
    qreal h1     = hue * 6;
    qreal x      = chroma * (1 - qAbs(std::fmod(h1, 2) - 1));
    QColor col;
    if (h1 >= 0 && h1 < 1)
        col = QColor::fromRgbF(chroma, x, 0);
    else if (h1 < 2)
        col = QColor::fromRgbF(x, chroma, 0);
    else if (h1 < 3)
        col = QColor::fromRgbF(0, chroma, x);
    else if (h1 < 4)
        col = QColor::fromRgbF(0, x, chroma);
    else if (h1 < 5)
        col = QColor::fromRgbF(x, 0, chroma);
    else if (h1 < 6)
        col = QColor::fromRgbF(chroma, 0, x);

    qreal m = lig - chroma / 2;

    return QColor::fromRgbF(qBound(0.0, col.redF() + m, 1.0), qBound(0.0, col.greenF() + m, 1.0),
                            qBound(0.0, col.blueF() + m, 1.0), alpha);
}

} // namespace detail
} // namespace color_widgets
