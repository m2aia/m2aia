/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#include "m2FeatureListsView.h"

#include <QAction>
#include <QColorDialog>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QColor>

#include <mitkDataNode.h>
#include <mitkDataStorage.h>
#include <mitkIOUtil.h>
#include <m2UIUtils.h>

#include <itksys/Directory.hxx>
#include <itksys/SystemTools.hxx>

#include <fstream>
#include <limits>
#include <map>
#include <set>

const std::string m2FeatureListsView::VIEW_ID = "org.mitk.views.m2.featurelists";

// Column indices
enum Columns
{
  COL_NODE = 0,
  COL_INDEX,
  COL_MZ_MEAN,
  COL_MZ_MIN,
  COL_MZ_MAX,
  COL_INTENSITY_MEAN,
  COL_DESCRIPTION,
  COL_COLOR,
  COL_CCS,
  COL_COUNT
};

void m2FeatureListsView::CreateQtPartControl(QWidget *parent)
{
  auto *layout = new QVBoxLayout(parent);
  layout->setContentsMargins(4, 4, 4, 4);
  layout->setSpacing(4);

  m_InfoLabel = new QLabel("Select IntervalVector nodes in the Data Manager.", parent);
  m_InfoLabel->setWordWrap(true);
  layout->addWidget(m_InfoLabel);

  m_Table = new QTableWidget(0, COL_COUNT, parent);
  m_Table->setHorizontalHeaderLabels(
    {"Node", "#", "m/z (mean)", "m/z (min)", "m/z (max)", "Intensity (mean)", "Description", "Color", "CCS range"});
  m_Table->horizontalHeader()->setStretchLastSection(false);
  m_Table->horizontalHeader()->setSectionResizeMode(COL_NODE, QHeaderView::ResizeToContents);
  m_Table->horizontalHeader()->setSectionResizeMode(COL_INDEX, QHeaderView::ResizeToContents);
  m_Table->horizontalHeader()->setSectionResizeMode(COL_MZ_MEAN, QHeaderView::ResizeToContents);
  m_Table->horizontalHeader()->setSectionResizeMode(COL_MZ_MIN, QHeaderView::ResizeToContents);
  m_Table->horizontalHeader()->setSectionResizeMode(COL_MZ_MAX, QHeaderView::ResizeToContents);
  m_Table->horizontalHeader()->setSectionResizeMode(COL_INTENSITY_MEAN, QHeaderView::ResizeToContents);
  m_Table->horizontalHeader()->setSectionResizeMode(COL_DESCRIPTION, QHeaderView::Stretch);
  m_Table->horizontalHeader()->setSectionResizeMode(COL_COLOR, QHeaderView::ResizeToContents);
  m_Table->horizontalHeader()->setSectionResizeMode(COL_CCS, QHeaderView::ResizeToContents);
  // Description is editable; all other columns are read-only via item flags.
  m_Table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::AnyKeyPressed);
  m_Table->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_Table->setAlternatingRowColors(true);
  m_Table->hideColumn(COL_NODE);
  m_Table->hideColumn(COL_MZ_MIN);
  m_Table->hideColumn(COL_MZ_MAX);
  m_Table->hideColumn(COL_INTENSITY_MEAN);
  m_Table->hideColumn(COL_CCS);
  m_Table->verticalHeader()->setVisible(false);

  m_Table->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_Table->horizontalHeader(), &QHeaderView::customContextMenuRequested,
          this, &m2FeatureListsView::OnHeaderContextMenu);
  connect(m_Table, &QTableWidget::itemChanged, this, &m2FeatureListsView::OnItemChanged);
  connect(m_Table, &QTableWidget::cellDoubleClicked, this, &m2FeatureListsView::OnCellDoubleClicked);
  connect(m_Table, &QTableWidget::cellClicked, this, &m2FeatureListsView::OnCellClicked);
  connect(m_Table, &QTableWidget::currentCellChanged, this, &m2FeatureListsView::OnCurrentCellChanged);
  layout->addWidget(m_Table);
}

void m2FeatureListsView::SetFocus()
{
  if (m_Table)
    m_Table->setFocus();
}

void m2FeatureListsView::OnSelectionChanged(berry::IWorkbenchPart::Pointer /*source*/,
                                             const QList<mitk::DataNode::Pointer> &nodes)
{
  if(!nodes.empty() && dynamic_cast<m2::IntervalVector *>(nodes.front()->GetData()))
    PopulateTable(nodes);
}

void m2FeatureListsView::PopulateTable(const QList<mitk::DataNode::Pointer> &nodes)
{
  m_Table->blockSignals(true);
  m_Table->setRowCount(0);

  int totalIntervals = 0;

  for (const auto &node : nodes)
  {
    if (node.IsNull())
      continue;

    auto *iv = dynamic_cast<m2::IntervalVector *>(node->GetData());
    if (!iv)
      continue;

    const std::string nodeName = node->GetName();
    const auto &intervals = iv->GetIntervals();

    for (std::size_t i = 0; i < intervals.size(); ++i)
    {
      const m2::Interval &interval = intervals[i];
      const int row = m_Table->rowCount();
      m_Table->insertRow(row);

      // Read-only item helper
      auto makeItem = [](const QString &text, Qt::Alignment align = Qt::AlignRight | Qt::AlignVCenter) -> QTableWidgetItem *
      {
        auto *item = new QTableWidgetItem(text);
        item->setTextAlignment(align);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        return item;
      };

      // Node name (hidden column — used as key for write-back)
      m_Table->setItem(row, COL_NODE, makeItem(QString::fromStdString(nodeName), Qt::AlignLeft | Qt::AlignVCenter));

      // Index (1-based display); 0-based index stored in UserRole for write-back
      auto *indexItem = makeItem(QString::number(static_cast<int>(i + 1)));
      indexItem->setData(Qt::UserRole, static_cast<int>(i));
      m_Table->setItem(row, COL_INDEX, indexItem);

      // m/z statistics (read-only)
      auto safeVal = [](const m2::Accumulator &a, auto fmt) -> QString
      {
        return a.count() > 0 ? QString::number(fmt(a), 'f', 4) : QString("-");
      };
      m_Table->setItem(row, COL_MZ_MEAN, makeItem(safeVal(interval.x, [](const m2::Accumulator &a){ return a.mean(); })));
      m_Table->setItem(row, COL_MZ_MIN,  makeItem(safeVal(interval.x, [](const m2::Accumulator &a){ return a.min();  })));
      m_Table->setItem(row, COL_MZ_MAX,  makeItem(safeVal(interval.x, [](const m2::Accumulator &a){ return a.max();  })));
      m_Table->setItem(row, COL_INTENSITY_MEAN,
        makeItem(safeVal(interval.y, [](const m2::Accumulator &a){ return a.mean(); })));

      // Description — editable (default item flags include Qt::ItemIsEditable)
      auto *descItem = new QTableWidgetItem(QString::fromStdString(interval.description));
      descItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
      m_Table->setItem(row, COL_DESCRIPTION, descItem);

      // Color swatch — read-only text; editable via double-click color picker
      auto *colorItem = makeItem(QString::fromStdString(interval.color), Qt::AlignHCenter | Qt::AlignVCenter);
      if (!interval.color.empty())
      {
        QColor c(QString::fromStdString(interval.color));
        if (c.isValid())
          colorItem->setBackground(c);
      }
      m_Table->setItem(row, COL_COLOR, colorItem);

      ++totalIntervals;
    }
  }

  m_Table->blockSignals(false);

  if (m_InfoLabel)
  {
    if (totalIntervals == 0)
      m_InfoLabel->setText("Select IntervalVector nodes in the Data Manager.");
    else
      m_InfoLabel->setText(QString("%1 interval(s) from %2 node(s).")
                             .arg(totalIntervals)
                             .arg(nodes.size()));
  }

  ScanFeatureNrrdFolder(nodes);
}

void m2FeatureListsView::ScanFeatureNrrdFolder(const QList<mitk::DataNode::Pointer> &nodes)
{
  // Feature record collected from a single .nrrd file's header.
  struct FeatureRecord
  {
    int         id           = -1;
    double      mz_low       = 0.0;
    double      mz_high      = 0.0;
    double      mz_centroid  = 0.0;
    double      ccs_low      = 0.0;
    double      ccs_high     = 0.0;
    std::string name;
    std::string nrrdFile;
  };

  for (const auto &node : nodes)
  {
    if (node.IsNull())
      continue;

    auto *iv = dynamic_cast<m2::IntervalVector *>(node->GetData());
    if (!iv)
      continue;

    const auto &intervals = iv->GetIntervals();
    if (intervals.empty())
      continue;

    // ── 1. Guard: proceed only when the vector contains duplicate x.mean() values ──
    {
      std::set<double> seen;
      bool hasIdentical = false;
      for (const auto &interval : intervals)
      {
        if (!seen.insert(interval.x.mean()).second)
        {
          hasIdentical = true;
          break;
        }
      }
      if (!hasIdentical)
        continue;
    }

    // ── 2. Walk up to the parent node ────────────────────────────────────────────
    auto sources = GetDataStorage()->GetSources(node);
    if (!sources || sources->empty())
    {
      MITK_WARN << "[FeatureLists] '" << node->GetName() << "' has no parent — skipping folder scan.";
      continue;
    }
    const mitk::DataNode *parentNode = sources->front();

    // ── 3. Retrieve the original file path stored by MITK's IO infrastructure ───
    auto inputLocationProp = parentNode->GetData()->GetProperty("m2aia.IO.path");
    if (!inputLocationProp)
    {
      MITK_WARN << "[FeatureLists] Parent '" << parentNode->GetName()
                << "' has no inputlocation — skipping folder scan.";
      continue;
    }
    std::string inputLocation = inputLocationProp->GetValueAsString();
    MITK_INFO << "[FeatureLists] Parent '" << parentNode->GetName()
                << "' input location: " << inputLocation;

    // ── 4. Build <dir>/<basename_without_extension> as the feature folder path ──
    const std::string baseName = itksys::SystemTools::GetFilenameWithoutLastExtension(inputLocation);
    const std::string dirPath  = itksys::SystemTools::GetFilenamePath(inputLocation);
    const std::string folderPath = dirPath + "/" + baseName;

    if (!itksys::SystemTools::FileIsDirectory(folderPath))
    {
      MITK_INFO << "[FeatureLists] Feature folder not found: " << folderPath;
      continue;
    }

    MITK_INFO << "[FeatureLists] Scanning feature folder: " << folderPath;

    // ── 5. Iterate .nrrd files in the folder ─────────────────────────────────────
    std::vector<FeatureRecord> records;

    itksys::Directory directory;
    if (!directory.Load(folderPath.c_str()))
      continue;

    for (unsigned long fi = 0; fi < directory.GetNumberOfFiles(); ++fi)
    {
      const std::string fname = directory.GetFile(fi);
      if (itksys::SystemTools::GetFilenameLastExtension(fname) != ".nrrd")
        continue;

      const std::string fullPath = folderPath + "/" + fname;

      // ── 6. Parse the NRRD header: key:=value pairs stop at the first blank line ──
      std::ifstream in(fullPath);
      if (!in.is_open())
        continue;

      std::map<std::string, std::string> kv;
      std::string line;
      while (std::getline(in, line))
      {
        if (line.empty() || line == "\r")
          break; // blank line = end of NRRD header

        const auto sep = line.find(":=");
        if (sep == std::string::npos)
          continue; // not a key-value pair

        std::string key = line.substr(0, sep);
        std::string val = line.substr(sep + 2);

        // trim leading/trailing whitespace
        const auto lstrip = [](std::string &s)
        {
          s.erase(0, s.find_first_not_of(" \t\r\n"));
        };
        const auto rstrip = [](std::string &s)
        {
          const auto pos = s.find_last_not_of(" \t\r\n");
          if (pos != std::string::npos) s.erase(pos + 1);
        };
        lstrip(key); rstrip(key);
        lstrip(val); rstrip(val);

        if (!key.empty())
          kv[key] = val;
      }
      in.close();

      // ── 7. Extract the seven known fields (skip files that have none) ───────────
      if (!kv.count("feature_id")       && !kv.count("feature_mz_low") &&
          !kv.count("feature_mz_high")  && !kv.count("feature_ccs_low") && 
          !kv.count("feature_ccs_high") && !kv.count("feature_name"))
        continue;

      FeatureRecord rec;
      rec.nrrdFile = fname;
      try
      {
        if (kv.count("feature_id"))          rec.id          = std::stoi(kv["feature_id"]);
        if (kv.count("feature_mz_low"))       rec.mz_low      = std::stod(kv["feature_mz_low"]);
        if (kv.count("feature_mz_high"))      rec.mz_high     = std::stod(kv["feature_mz_high"]);
        if (kv.count("feature_ccs_low"))      rec.ccs_low     = std::stod(kv["feature_ccs_low"]);
        if (kv.count("feature_ccs_high"))     rec.ccs_high    = std::stod(kv["feature_ccs_high"]);
        if (kv.count("feature_name"))         rec.name        = kv["feature_name"];
        
        rec.mz_centroid = (rec.mz_low + rec.mz_high) * 0.5;
      }
      catch (const std::exception &e)
      {
        MITK_WARN << "[FeatureLists] Could not parse fields in '" << fname << "': " << e.what();
        continue;
      }
      records.push_back(std::move(rec));
    }

    // ── 8. Print collected records and apply them to the table ──────────────────
    // MITK_INFO << "[FeatureLists] Node '" << node->GetName() << "': "
    //           << records.size() << " feature record(s) found in " << folderPath;
    // for (const auto &r : records)
    // {
    //   MITK_INFO << "  [" << r.id << "] "
    //             << "name='" << r.name << "'  "
    //             << "mz=[" << r.mz_low << ", " << r.mz_centroid << ", " << r.mz_high << "]  "
    //             << "ccs=[" << r.ccs_low << ", " << r.ccs_high << "]  "
    //             << "file=" << r.nrrdFile;
    // }

    // For each feature record, find the table row for this node whose x.mean() is
    // closest to mz_centroid AND whose description cell is currently empty.
    const std::string nodeNameStr = node->GetName();
    bool anyCcs = false;
    std::set<int> usedRows; // table row indices already assigned
    m_Table->blockSignals(true);

    for (const auto &rec : records)
    {
      int bestRow  = -1;
      double bestDiff = std::numeric_limits<double>::max();

      for (int row = 0; row < m_Table->rowCount(); ++row)
      {
        if (usedRows.count(row))
          continue;

        // Skip rows that already have a description
        auto *descItem = m_Table->item(row, COL_DESCRIPTION);
        if (descItem && !descItem->text().isEmpty())
          continue;
        auto *mzMeanItem = m_Table->item(row, COL_MZ_MEAN);
        const double xMean = mzMeanItem ? mzMeanItem->text().toDouble() : 0.0;
        MITK_INFO << "Comparing feature '" << rec.name << "' (m/z centroid: " << rec.mz_centroid
                  << ") to row " << row << " (m/z mean: " << xMean << ")";  
        const double diff = std::abs(rec.mz_centroid - xMean);
        if (diff < bestDiff && diff < 0.01) { bestDiff = diff; bestRow = row; }
      }

      if (bestRow < 0)
        continue;
      MITK_INFO << "Assigning feature '" << rec.name << "' to row " << bestRow
                  << " (m/z mean: " << m_Table->item(bestRow, COL_MZ_MEAN)->text() << ")";
      usedRows.insert(bestRow);

      // Overwrite description cell and the underlying IntervalVector
      auto *descItem = m_Table->item(bestRow, COL_DESCRIPTION);
      if (descItem)
        descItem->setText(QString::fromStdString(rec.name));

      // CCS range cell — full NRRD path stored in Qt::UserRole for double-click
      const QString ccsText = QString("[%1, %2]")
        .arg(rec.ccs_low,  0, 'f', 4)
        .arg(rec.ccs_high, 0, 'f', 4);
      auto *ccsItem = new QTableWidgetItem(ccsText);
      ccsItem->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
      ccsItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      ccsItem->setData(Qt::UserRole, QString::fromStdString(folderPath + "/" + rec.nrrdFile));
      m_Table->setItem(bestRow, COL_CCS, ccsItem);
      anyCcs = true;
    }
    m_Table->blockSignals(false);

    if (anyCcs)
      m_Table->showColumn(COL_CCS);
  }
}

void m2FeatureListsView::OnHeaderContextMenu(const QPoint &pos)
{
  struct ColToggle { int col; QString label; };
  static const ColToggle toggles[] = {
    { COL_NODE,          "Node Name"        },
    { COL_MZ_MIN,        "m/z (min)"        },
    { COL_MZ_MAX,        "m/z (max)"        },
    { COL_INTENSITY_MEAN,"Intensity (mean)" },
    { COL_CCS,           "CCS range"        },
  };

  QMenu menu(m_Table);
  for (const auto &t : toggles)
  {
    auto *act = menu.addAction(t.label);
    act->setCheckable(true);
    act->setChecked(!m_Table->isColumnHidden(t.col));
    act->setData(t.col);
  }

  QAction *triggered = menu.exec(m_Table->horizontalHeader()->mapToGlobal(pos));
  if (triggered)
  {
    const int col = triggered->data().toInt();
    m_Table->setColumnHidden(col, !triggered->isChecked());
  }
}

void m2FeatureListsView::OnCurrentCellChanged(int row, int /*col*/, int /*prevRow*/, int /*prevCol*/)
{
  OnCellClicked(row, 0); // Column is ignored in OnCellClicked, so we can pass any value here.
}

void m2FeatureListsView::OnCellClicked(int row, int /*col*/)
{
  auto *mzMinItem  = m_Table->item(row, COL_MZ_MIN);
  auto *mzMaxItem  = m_Table->item(row, COL_MZ_MAX);
  if (!mzMinItem || !mzMaxItem)
    return;

  bool okMin = false, okMax = false;
  const double lower = mzMinItem->text().toDouble(&okMin);
  const double upper = mzMaxItem->text().toDouble(&okMax);
  if (!okMin || !okMax)
    return;

  if (lower == upper){
    emit m2::UIUtils::Instance()->UpdateImage(lower, -1);
    return;
  }
  
  
  const double center = (lower + upper) * 0.5;
  const double tol    = (upper - lower) * 0.5;
  emit m2::UIUtils::Instance()->UpdateImage(center, tol);
}

void m2FeatureListsView::OnItemChanged(QTableWidgetItem *item)
{
  if (item->column() != COL_DESCRIPTION)
    return;

  const int row = item->row();
  auto *nodeItem = m_Table->item(row, COL_NODE);
  auto *indexItem = m_Table->item(row, COL_INDEX);
  if (!nodeItem || !indexItem)
    return;

  const std::string nodeName = nodeItem->text().toStdString();
  const int intervalIdx = indexItem->data(Qt::UserRole).toInt();

  auto nodes = GetDataStorage()->GetAll();
  for (auto it = nodes->begin(); it != nodes->end(); ++it)
  {
    if ((*it)->GetName() != nodeName)
      continue;
    auto *iv = dynamic_cast<m2::IntervalVector *>((*it)->GetData());
    if (iv && intervalIdx >= 0 && static_cast<std::size_t>(intervalIdx) < iv->GetIntervals().size())
      iv->GetIntervals()[intervalIdx].description = item->text().toStdString();
    break;
  }
}

void m2FeatureListsView::OnCellDoubleClicked(int row, int col)
{
  if (col == COL_CCS)
  {
    auto *ccsItem = m_Table->item(row, COL_CCS);
    if (!ccsItem)
      return;
    const std::string nrrdPath = ccsItem->data(Qt::UserRole).toString().toStdString();
    if (nrrdPath.empty() || !itksys::SystemTools::FileExists(nrrdPath))
      return;

    // Find the IntervalVector node, then walk up to the imzML parent
    auto *nodeNameItem = m_Table->item(row, COL_NODE);
    if (!nodeNameItem)
      return;
    const std::string ivNodeName = nodeNameItem->text().toStdString();

    mitk::DataNode::Pointer imzmlParent;
    auto allNodes = GetDataStorage()->GetAll();
    for (auto it = allNodes->begin(); it != allNodes->end(); ++it)
    {
      if ((*it)->GetName() == ivNodeName && dynamic_cast<m2::IntervalVector *>((*it)->GetData()))
      {
        auto sources = GetDataStorage()->GetSources(*it);
        if (sources && !sources->empty())
          imzmlParent = sources->front();
        break;
      }
    }

    // Avoid loading the same NRRD twice
    const std::string childName = itksys::SystemTools::GetFilenameWithoutLastExtension(nrrdPath);
    if (imzmlParent)
    {
      auto children = GetDataStorage()->GetDerivations(imzmlParent);
      for (auto it = children->begin(); it != children->end(); ++it)
        if ((*it)->GetName() == childName)
          return;
    }

    auto dataList = mitk::IOUtil::Load(nrrdPath);
    if (dataList.empty())
      return;


      
    auto childNode = mitk::DataNode::New();
    mitk::BaseData::Pointer data = dataList.front();
    data->GetGeometry()->SetOrigin(imzmlParent->GetData()->GetGeometry()->GetOrigin());
    data->GetGeometry()->SetSpacing(imzmlParent->GetData()->GetGeometry()->GetSpacing());
    data->GetGeometry()->SetIndexToWorldTransformByVtkMatrix(imzmlParent->GetData()->GetGeometry()->GetVtkMatrix());
    childNode->SetData(data);
    childNode->SetName(childName);
    GetDataStorage()->Add(childNode, imzmlParent);
    return;
  }

  if (col != COL_COLOR)
    return;

  auto *nodeItem = m_Table->item(row, COL_NODE);
  auto *indexItem = m_Table->item(row, COL_INDEX);
  auto *colorItem = m_Table->item(row, COL_COLOR);
  if (!nodeItem || !indexItem || !colorItem)
    return;

  const std::string nodeName = nodeItem->text().toStdString();
  const int intervalIdx = indexItem->data(Qt::UserRole).toInt();

  QColor initial = colorItem->background().color();
  if (!initial.isValid())
    initial = Qt::white;

  const QColor chosen = QColorDialog::getColor(initial, m_Table, "Select Interval Color");
  if (!chosen.isValid())
    return;

  m_Table->blockSignals(true);
  colorItem->setBackground(chosen);
  colorItem->setText(chosen.name());
  m_Table->blockSignals(false);

  auto nodes = GetDataStorage()->GetAll();
  for (auto it = nodes->begin(); it != nodes->end(); ++it)
  {
    if ((*it)->GetName() != nodeName)
      continue;
    auto *iv = dynamic_cast<m2::IntervalVector *>((*it)->GetData());
    if (iv && intervalIdx >= 0 && static_cast<std::size_t>(intervalIdx) < iv->GetIntervals().size())
      iv->GetIntervals()[intervalIdx].color = chosen.name().toStdString();
    break;
  }
}
