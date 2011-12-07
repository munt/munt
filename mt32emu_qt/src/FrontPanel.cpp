/* Copyright (C) 2005 Dean Beeler, 2011 Jerome Fisher, Sergey V. Mikayev
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QtGui>

#include "FrontPanel.h"
#include "ui_FrontPanel.h"

#include "font_6x8.h"

FrontPanel::FrontPanel(QWidget *parent, SynthRoute *useSynthRoute) :
		QDialog(parent),
		ui(new Ui::FrontPanel),
		synthRoute(useSynthRoute),
		drawMaskedChars(true),
		volume(100)
{
	ui->setupUi(this);
	strncpy(lcdText, "1 2 3 4 5 R |vol:100", 20);
	for (int i = 0; i < 20; i++) {
		maskedChar[i] = false;
	}
}

FrontPanel::~FrontPanel()
{
	delete ui;
}

void FrontPanel::paintEvent(QPaintEvent *)
{
	QPainter painter(this);

	painter.drawPixmap(76, 58, QPixmap(":images/LCDOn.gif"));

	//QPixmap drawBuffer(680, 57);
	QPixmap drawBuffer(120, 10);
	QPainter lcdPainter(&drawBuffer);
	//lcdPainter.drawPixmap(QRect(0, 0, 680, 57), QPixmap(":images/LCDOn.gif"), QRect(7, 7, 171, 15));
	lcdPainter.drawPixmap(QRect(0, 0, 120, 10), QPixmap(":images/LCDOn.gif"), QRect(7, 7, 171, 15));

	QColor bgColor(98, 127, 0);
	QColor fgColor(232, 254, 0);

	int xat, xstart, yat, len;
	xstart = 0;
	yat = 0;
	len = strlen(lcdText);

	for (int i = 0; i < 20; i++) {
		unsigned char c;
		c = 0x20;
		if (i < len) {
			c = lcdText[i];
		}

		// Don't render characters we don't have mapped
		if(c < 0x20) c = 0x20;
		if(c > 0x7f) c = 0x20;

		c -= 0x20;

		yat = 1;
		for (int t = 0; t < 8; t++) {
			xat = xstart;
			unsigned char fval;
			if (maskedChar[i] && (t != 7) && drawMaskedChars) {
				fval = 0x1f;
			} else {
				fval = Font_6x8[c][t];
			}
			for (int m = 4; m >= 0; --m) {
				if ((fval >> m) & 1) {
					//lcdPainter.fillRect(xat, yat, 5, 5, fgColor);
					lcdPainter.fillRect(xat, yat, 1, 1, fgColor);
				} else {
					//lcdPainter.fillRect(xat, yat, 5, 5, bgColor);
					lcdPainter.fillRect(xat, yat, 1, 1, bgColor);
				}
				//xat += 6;
				xat += 1;
			}
			//yat += 6;
			//if (t == 6) yat += 6;
			yat += 1;
			if (t == 6) yat += 1;
		}
		//xstart += 34;
		xstart += 6;
	}
	// FIXME: it'll be great to improve scaling quality
	//painter.drawPixmap(QRect(76 + 7, 58 + 7, 680, 57), drawBuffer);	// huge non-scaled
	//painter.drawPixmap(QRect(76 + 7, 58 + 7, 171, 15), drawBuffer);	// nearest neighbor
	painter.drawImage(76 + 7, 58 + 7, drawBuffer.toImage().scaled(171, 15, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)); // bilinear filter

	QPixmap volumeKnob(":images/VolumeKnob.png");
	QPixmap rotatedKnob(volumeKnob);
	QPainter knobPainter(&rotatedKnob);
	knobPainter.translate(25, 25);
	knobPainter.rotate(3 * volume);
	knobPainter.translate(-25, -25);
	knobPainter.drawPixmap(0, 0, volumeKnob);
	painter.drawPixmap(706, 48, rotatedKnob);
}
