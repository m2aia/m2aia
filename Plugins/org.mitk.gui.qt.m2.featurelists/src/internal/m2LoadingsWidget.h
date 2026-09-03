/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#pragma once

#include <QColor>
#include <QWidget>

#include <vector>

class QPainter;

/**
  \brief A miniature plot of the loadings one analysis gives to one centroid, meant for a single
  table cell.

  The components are drawn as bars along a zero baseline, upwards for a positive loading and
  downwards for a negative one, each as long as the loading is strong relative to the largest one
  that component reaches anywhere in the table. Reading a whole column of these cells shows at a
  glance which centroids a component picks out and with which sign, which a row of numbers does
  not.

  Length carries the magnitude and the colour carries the sign; the two say the same thing on
  purpose, so a bar stays legible when it is only a few pixels tall.

  Which colours those are depends on the method the loadings came from, see ColorTableForMethod:
  a method always looks the same, so a column can be recognised as a PCA or a PLS-DA without
  reading its header.

  The height the widget reports never depends on how many components it shows - the bars share the
  width instead - so a table row keeps its ordinary height however many components an analysis
  produced.

  One widget shows one analysis. Several analyses belong in several cells, so that their loadings
  are compared against their own scale rather than against each other.

  Use it with QTableWidget::setCellWidget, or call the static Paint() to draw the same picture
  without creating a widget per row.
*/
class m2LoadingsWidget : public QWidget
{
  Q_OBJECT

public:
  /** The colours one analysis is drawn with.

      A method that cannot produce a negative loading, such as a non-negative factorization, is
      marked unsigned: its bars then rise from the bottom of the cell rather than from a line
      through the middle, which uses the whole height instead of leaving the lower half empty. */
  struct ColorTable
  {
    QColor positive;
    QColor negative;
    bool signedValues = true;
  };

  /** The loading of one component for one centroid. */
  struct Entry
  {
    /** Full name of the feature, shown in the tooltip. */
    QString name;
    double value = 0.0;
    /** The value is divided by this before it is drawn, so that a component stays comparable
        between centroids. A value of zero or less draws nothing. */
    double scale = 1.0;
    /** False when this centroid does not carry the feature, which is left as a gap on the
        baseline rather than drawn as a zero it never produced. */
    bool valid = false;
  };

  explicit m2LoadingsWidget(QWidget *parent = nullptr);

  void SetEntries(std::vector<Entry> entries);
  const std::vector<Entry> &GetEntries() const { return m_Entries; }

  void SetColorTable(const ColorTable &table);
  const ColorTable &GetColorTable() const { return m_ColorTable; }

  /** The table a method is always drawn with, chosen from its name.

      The methods of the Data Compression view have a table of their own so that the same method
      looks the same in every session and in every table. A name that is not among them gets a
      table derived from the name itself, which is at least stable from one session to the next. */
  static ColorTable ColorTableForMethod(const QString &methodName);

  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;

  /** Draws the plot into the given rectangle. */
  static void Paint(QPainter &painter,
                    const QRect &area,
                    const std::vector<Entry> &entries,
                    const ColorTable &colors);

protected:
  void paintEvent(QPaintEvent *event) override;
  /** Shows the name and the value of the bar under the cursor. */
  bool event(QEvent *event) override;

private:
  /** The entry drawn at the given position, or -1. */
  int IndexAt(const QPoint &position) const;

  std::vector<Entry> m_Entries;
  ColorTable m_ColorTable = ColorTableForMethod(QString());
};
