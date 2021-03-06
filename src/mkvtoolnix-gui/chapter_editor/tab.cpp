#include "common/common_pch.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>
#include <QTimer>

#include <matroska/KaxSemantic.h>

#include "common/bitvalue.h"
#include "common/chapters/chapters.h"
#include "common/construct.h"
#include "common/ebml.h"
#include "common/mm_io_x.h"
#include "common/mpls.h"
#include "common/qt.h"
#include "common/segmentinfo.h"
#include "common/segment_tracks.h"
#include "common/strings/formatting.h"
#include "common/strings/parsing.h"
#include "common/translation.h"
#include "common/xml/ebml_chapters_converter.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/forms/chapter_editor/tab.h"
#include "mkvtoolnix-gui/chapter_editor/name_model.h"
#include "mkvtoolnix-gui/chapter_editor/mass_modification_dialog.h"
#include "mkvtoolnix-gui/chapter_editor/tab.h"
#include "mkvtoolnix-gui/chapter_editor/tool.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/util/message_box.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/util.h"

namespace mtx { namespace gui { namespace ChapterEditor {

using namespace mtx::gui;

Tab::Tab(QWidget *parent,
         QString const &fileName)
  : QWidget{parent}
  , ui{new Ui::Tab}
  , m_fileName{fileName}
  , m_chapterModel{new ChapterModel{this}}
  , m_nameModel{new NameModel{this}}
  , m_expandAllAction{new QAction{this}}
  , m_collapseAllAction{new QAction{this}}
  , m_addEditionBeforeAction{new QAction{this}}
  , m_addEditionAfterAction{new QAction{this}}
  , m_addChapterBeforeAction{new QAction{this}}
  , m_addChapterAfterAction{new QAction{this}}
  , m_addSubChapterAction{new QAction{this}}
  , m_removeElementAction{new QAction{this}}
  , m_duplicateAction{new QAction{this}}
  , m_massModificationAction{new QAction{this}}
{
  // Setup UI controls.
  ui->setupUi(this);

  setupUi();

  retranslateUi();
}

Tab::~Tab() {
}

void
Tab::setupUi() {
  ui->elements->setModel(m_chapterModel);
  ui->tvChNames->setModel(m_nameModel);

  ui->cbChNameCountry ->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

  Util::setupLanguageComboBox(*ui->cbChNameLanguage);
  Util::setupCountryComboBox(*ui->cbChNameCountry, Q(""), true);

  m_nameWidgets << ui->pbChRemoveName
                << ui->lChName         << ui->leChName
                << ui->lChNameLanguage << ui->cbChNameLanguage
                << ui->lChNameCountry  << ui->cbChNameCountry;

  connect(ui->elements,                    &Util::BasicTreeView::customContextMenuRequested,                       this, &Tab::showChapterContextMenu);
  connect(ui->elements->selectionModel(),  &QItemSelectionModel::selectionChanged,                                 this, &Tab::chapterSelectionChanged);
  connect(ui->tvChNames->selectionModel(), &QItemSelectionModel::selectionChanged,                                 this, &Tab::nameSelectionChanged);
  connect(ui->leChName,                    &QLineEdit::textEdited,                                                 this, &Tab::chapterNameEdited);
  connect(ui->cbChNameLanguage,            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &Tab::chapterNameLanguageChanged);
  connect(ui->cbChNameCountry,             static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &Tab::chapterNameCountryChanged);
  connect(ui->pbChAddName,                 &QPushButton::clicked,                                                  this, &Tab::addChapterName);
  connect(ui->pbChRemoveName,              &QPushButton::clicked,                                                  this, &Tab::removeChapterName);

  connect(m_expandAllAction,               &QAction::triggered,                                                    this, &Tab::expandAll);
  connect(m_collapseAllAction,             &QAction::triggered,                                                    this, &Tab::collapseAll);
  connect(m_addEditionBeforeAction,        &QAction::triggered,                                                    this, &Tab::addEditionBefore);
  connect(m_addEditionAfterAction,         &QAction::triggered,                                                    this, &Tab::addEditionAfter);
  connect(m_addChapterBeforeAction,        &QAction::triggered,                                                    this, &Tab::addChapterBefore);
  connect(m_addChapterAfterAction,         &QAction::triggered,                                                    this, &Tab::addChapterAfter);
  connect(m_addSubChapterAction,           &QAction::triggered,                                                    this, &Tab::addSubChapter);
  connect(m_removeElementAction,           &QAction::triggered,                                                    this, &Tab::removeElement);
  connect(m_duplicateAction,               &QAction::triggered,                                                    this, &Tab::duplicateElement);
  connect(m_massModificationAction,        &QAction::triggered,                                                    this, &Tab::massModify);
}

void
Tab::updateFileNameDisplay() {
  if (!m_fileName.isEmpty()) {
    auto info = QFileInfo{m_fileName};
    ui->fileName->setText(info.fileName());
    ui->directory->setText(info.path());

  } else {
    ui->fileName->setText(QY("<unsaved file>"));
    ui->directory->setText(Q(""));

  }
}

void
Tab::retranslateUi() {
  ui->retranslateUi(this);

  updateFileNameDisplay();

  m_expandAllAction->setText(QY("&Expand all"));
  m_collapseAllAction->setText(QY("&Collapse all"));
  m_addEditionBeforeAction->setText(QY("Add new e&dition before"));
  m_addEditionAfterAction->setText(QY("Add new ed&ition after"));
  m_addChapterBeforeAction->setText(QY("Add new c&hapter before"));
  m_addChapterAfterAction->setText(QY("Add new ch&apter after"));
  m_addSubChapterAction->setText(QY("Add new &sub-chapter inside"));
  m_removeElementAction->setText(QY("&Remove selected edition or chapter"));
  m_duplicateAction->setText(QY("D&uplicate selected edition or chapter"));
  m_massModificationAction->setText(QY("Additional &modifications"));

  m_chapterModel->retranslateUi();
  m_nameModel->retranslateUi();

  resizeChapterColumnsToContents();

  emit titleChanged();
}

void
Tab::resizeChapterColumnsToContents()
  const {
  Util::resizeViewColumnsToContents(ui->elements);
}

void
Tab::resizeNameColumnsToContents()
  const {
  Util::resizeViewColumnsToContents(ui->tvChNames);
}

QString
Tab::title()
  const {
  if (m_fileName.isEmpty())
    return QY("<unsaved file>");
  return QFileInfo{m_fileName}.fileName();
}

QString const &
Tab::fileName()
  const {
  return m_fileName;
}

void
Tab::newFile() {
  addEdition(false);

  auto selectionModel = ui->elements->selectionModel();
  auto selection      = QItemSelection{m_chapterModel->index(0, 0), m_chapterModel->index(0, m_chapterModel->columnCount() - 1)};
  selectionModel->select(selection, QItemSelectionModel::ClearAndSelect);

  addSubChapter();

  auto parentIdx = m_chapterModel->index(0, 0);
  selection      = QItemSelection{m_chapterModel->index(0, 0, parentIdx), m_chapterModel->index(0, m_chapterModel->columnCount() - 1, parentIdx)};
  selectionModel->select(selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Current);

  resizeChapterColumnsToContents();

  ui->leChStart->selectAll();
  ui->leChStart->setFocus();

  m_savedState = currentState();
}

void
Tab::resetData() {
  m_analyzer.reset();
  m_nameModel->reset();
  m_chapterModel->reset();
}

ChaptersPtr
Tab::loadFromMatroskaFile() {
  m_analyzer = std::make_unique<QtKaxAnalyzer>(this, m_fileName);

  if (!m_analyzer->process(kax_analyzer_c::parse_mode_fast)) {
    Util::MessageBox::critical(this, QY("File parsing failed"), QY("The file you tried to open (%1) could not be read successfully.").arg(m_fileName));
    emit removeThisTab();
    return {};
  }

  auto idx = m_analyzer->find(KaxChapters::ClassInfos.GlobalId);
  if (-1 == idx) {
    Util::MessageBox::critical(this, QY("File parsing failed"), QY("The file you tried to open (%1) does not contain any chapters.").arg(m_fileName));
    emit removeThisTab();
    return {};
  }

  auto chapters = m_analyzer->read_element(idx);
  if (!chapters) {
    Util::MessageBox::critical(this, QY("File parsing failed"), QY("The file you tried to open (%1) could not be read successfully.").arg(m_fileName));
    emit removeThisTab();
  }

  return std::static_pointer_cast<KaxChapters>(chapters);
}

ChaptersPtr
Tab::loadFromChapterFile() {
  auto isSimpleFormat = false;
  auto chapters       = ChaptersPtr{};
  auto error          = QString{};

  try {
    chapters = parse_chapters(to_utf8(m_fileName), 0, -1, 0, "", "", true, &isSimpleFormat);

  } catch (mtx::mm_io::exception &ex) {
    error = Q(ex.what());

  } catch (mtx::chapter_parser_x &ex) {
    error = Q(ex.what());
  }

  if (!chapters) {
    auto message = QY("The file you tried to open (%1) is recognized as neither a valid Matroska nor a valid chapter file.").arg(m_fileName);
    if (!error.isEmpty())
      message = Q("%1 %2").arg(message).arg(QY("Error message from the parser: %1").arg(error));

    Util::MessageBox::critical(this, QY("File parsing failed"), message);
    emit removeThisTab();

  } else if (isSimpleFormat) {
    Util::MessageBox::warning(this, QY("Simple chapter file format"), QY("The file you tried to open (%1) is a simple chapter file format. This can only be read but not written. "
                                                                         "You will have to save the file to a Matroska or an XML chapter file.").arg(m_fileName));
    m_fileName.clear();
    emit titleChanged();
  }

  return chapters;
}

ChaptersPtr
Tab::timecodesToChapters(std::vector<timecode_c> const &timecodes)
  const {
  auto &cfg     = Util::Settings::get();
  auto chapters = ChaptersPtr{ static_cast<KaxChapters *>(mtx::construct::cons<KaxChapters>(mtx::construct::cons<KaxEditionEntry>())) };
  auto &edition = GetChild<KaxEditionEntry>(*chapters);
  auto idx      = 0;

  for (auto const &timecode : timecodes) {
    auto nameTemplate = QString{ cfg.m_chapterNameTemplate };
    auto name         = to_wide(nameTemplate.replace(Q("<NUM>"), QString::number(++idx)));
    auto atom         = mtx::construct::cons<KaxChapterAtom>(new KaxChapterTimeStart, timecode.to_ns(),
                                                             mtx::construct::cons<KaxChapterDisplay>(new KaxChapterString,   name,
                                                                                                     new KaxChapterLanguage, to_utf8(cfg.m_defaultChapterLanguage)));
    if (!cfg.m_defaultChapterCountry.isEmpty())
      GetChild<KaxChapterCountry>(GetChild<KaxChapterDisplay>(atom)).SetValue(to_utf8(cfg.m_defaultChapterCountry));

    edition.PushElement(*atom);
  }

  return chapters;
}

ChaptersPtr
Tab::loadFromMplsFile() {
  auto chapters = ChaptersPtr{};
  auto error    = QString{};

  try {
    auto in     = mm_file_io_c{to_utf8(m_fileName)};
    auto parser = mpls::parser_c{};

    if (parser.parse(&in))
      chapters = timecodesToChapters(parser.get_chapters());

  } catch (mtx::mm_io::exception &ex) {
    error = Q(ex.what());

  } catch (mtx::mpls::exception &ex) {
    error = Q(ex.what());

  }

  if (!chapters) {
    auto message = QY("The file you tried to open (%1) is recognized as neither a valid Matroska nor a valid chapter file.").arg(m_fileName);
    if (!error.isEmpty())
      message = Q("%1 %2").arg(message).arg(QY("Error message from the parser: %1").arg(error));

    Util::MessageBox::critical(this, QY("File parsing failed"), message);
    emit removeThisTab();

  } else {
    m_fileName.clear();
    emit titleChanged();
  }

  return chapters;
}

void
Tab::load() {
  resetData();

  m_savedState  = currentState();
  auto chapters = kax_analyzer_c::probe(to_utf8(m_fileName)) ? loadFromMatroskaFile()
                : m_fileName.toLower().endsWith(Q(".mpls"))  ? loadFromMplsFile()
                :                                              loadFromChapterFile();

  if (!chapters)
    return;

  if (!m_fileName.isEmpty())
    m_fileModificationTime = QFileInfo{m_fileName}.lastModified();

  disconnect(m_chapterModel, &QStandardItemModel::rowsInserted, this, &Tab::expandInsertedElements);

  m_chapterModel->populate(*chapters);
  m_savedState = currentState();

  expandAll();
  resizeChapterColumnsToContents();

  connect(m_chapterModel, &QStandardItemModel::rowsInserted, this, &Tab::expandInsertedElements);
}

void
Tab::save() {
  if (!m_analyzer)
    saveAsXmlImpl(false);

  else
    saveToMatroskaImpl(false);
}

void
Tab::saveAsImpl(bool requireNewFileName,
                std::function<bool(bool, QString &)> const &worker) {
  if (!copyControlsToStorage())
    return;

  m_chapterModel->fixMandatoryElements();
  setControlsFromStorage();

  auto newFileName = m_fileName;
  if (m_fileName.isEmpty())
    requireNewFileName = true;

  if (!worker(requireNewFileName, newFileName))
    return;

  m_savedState = currentState();

  if (newFileName != m_fileName) {
    m_fileName             = newFileName;

    auto &settings         = Util::Settings::get();
    settings.m_lastOpenDir = QFileInfo{newFileName}.path();
    settings.save();

    updateFileNameDisplay();
    emit titleChanged();
  }

  MainWindow::get()->setStatusBarMessage(QY("The file has been saved successfully."));
}

void
Tab::saveAsXml() {
  saveAsXmlImpl(true);
}

void
Tab::saveAsXmlImpl(bool requireNewFileName) {
  saveAsImpl(requireNewFileName, [this](bool doRequireNewFileName, QString &newFileName) -> bool {
    if (doRequireNewFileName) {
      auto defaultFilePath = !m_fileName.isEmpty() ? QFileInfo{m_fileName}.path() : Util::Settings::get().m_lastOpenDir.path();
      newFileName          = QFileDialog::getSaveFileName(this, QY("Save chapters as XML"), defaultFilePath, QY("XML chapter files") + Q(" (*.xml);;") + QY("All files") + Q(" (*)"));

      if (newFileName.isEmpty())
        return false;
    }

    try {
      auto chapters = m_chapterModel->allChapters();
      auto out      = mm_file_io_c{to_utf8(newFileName), MODE_CREATE};
      mtx::xml::ebml_chapters_converter_c::write_xml(*chapters, out);

    } catch (mtx::mm_io::exception &) {
      Util::MessageBox::critical(this, QY("Saving failed"), QY("Creating the file failed. Check to make sure you have permission to write to that directory and that the drive is not full."));
      return false;

    } catch (mtx::xml::conversion_x &ex) {
      Util::MessageBox::critical(this, QY("Saving failed"), QY("Converting the chapters to XML failed: %1").arg(ex.what()));
      return false;
    }

    return true;
  });
}

void
Tab::saveToMatroska() {
  saveToMatroskaImpl(true);
}

void
Tab::saveToMatroskaImpl(bool requireNewFileName) {
  saveAsImpl(requireNewFileName, [this](bool doRequireNewFileName, QString &newFileName) -> bool {
    if (!m_analyzer)
      doRequireNewFileName = true;

    if (doRequireNewFileName) {
      auto defaultFilePath = !m_fileName.isEmpty() ? QFileInfo{m_fileName}.path() : Util::Settings::get().m_lastOpenDir.path();
      newFileName          = QFileDialog::getOpenFileName(this, QY("Save chapters to Matroska file"), defaultFilePath, QY("Matroska files") + Q(" (*.mkv *.mka *.mks *.mk3d);;") + QY("All files") + Q(" (*)"));

      if (newFileName.isEmpty())
        return false;
    }

    if (doRequireNewFileName || (QFileInfo{newFileName}.lastModified() != m_fileModificationTime)) {
      m_analyzer = std::make_unique<QtKaxAnalyzer>(this, newFileName);
      if (!m_analyzer->process(kax_analyzer_c::parse_mode_fast)) {
        Util::MessageBox::critical(this, QY("File parsing failed"), QY("The file you tried to open (%1) could not be read successfully.").arg(newFileName));
        return false;
      }
    }

    auto chapters = m_chapterModel->allChapters();
    auto result   = m_analyzer->update_element(chapters, true);
    m_analyzer->close_file();

    if (kax_analyzer_c::uer_success != result) {
      QtKaxAnalyzer::displayUpdateElementResult(this, result, QY("Saving the chapters failed."));
      return false;
    }

    m_fileModificationTime = QFileInfo{m_fileName}.lastModified();

    return true;
  });
}

void
Tab::selectChapterRow(QModelIndex const &idx,
                      bool ignoreSelectionChanges) {
  auto selection = QItemSelection{idx.sibling(idx.row(), 0), idx.sibling(idx.row(), 2)};

  m_ignoreChapterSelectionChanges = ignoreSelectionChanges;
  ui->elements->selectionModel()->setCurrentIndex(idx.sibling(idx.row(), 0), QItemSelectionModel::ClearAndSelect);
  ui->elements->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
  m_ignoreChapterSelectionChanges = false;
}

bool
Tab::copyControlsToStorage() {
  auto idx = Util::selectedRowIdx(ui->elements);
  return idx.isValid() ? copyControlsToStorage(idx) : true;
}

bool
Tab::copyControlsToStorage(QModelIndex const &idx) {
  auto result = copyControlsToStorageImpl(idx);
  if (result.first) {
    m_chapterModel->updateRow(idx);
    return true;
  }

  selectChapterRow(idx, true);

  Util::MessageBox::critical(this, QY("Validation failed"), result.second);

  return false;
}

Tab::ValidationResult
Tab::copyControlsToStorageImpl(QModelIndex const &idx) {
  auto stdItem = m_chapterModel->itemFromIndex(idx);
  if (!stdItem)
    return { true, QString{} };

  if (!idx.parent().isValid())
    return copyEditionControlsToStorage(m_chapterModel->editionFromItem(stdItem));
  return copyChapterControlsToStorage(m_chapterModel->chapterFromItem(stdItem));
}

Tab::ValidationResult
Tab::copyChapterControlsToStorage(ChapterPtr const &chapter) {
  if (!chapter)
    return { true, QString{} };

  auto uid = uint64_t{};

  if (!ui->leChUid->text().isEmpty()) {
    auto ok = false;
    uid     = ui->leChUid->text().toULongLong(&ok);
    if (!ok)
      return { false, QY("The chapter UID must be a number if given.") };
  }

  if (uid)
    GetChild<KaxChapterUID>(*chapter).SetValue(uid);
  else
    DeleteChildren<KaxChapterUID>(*chapter);

  if (!ui->cbChFlagEnabled->isChecked())
    GetChild<KaxChapterFlagEnabled>(*chapter).SetValue(0);
  else
    DeleteChildren<KaxChapterFlagEnabled>(*chapter);

  if (ui->cbChFlagHidden->isChecked())
    GetChild<KaxChapterFlagHidden>(*chapter).SetValue(1);
  else
    DeleteChildren<KaxChapterFlagHidden>(*chapter);

  auto startTimecode = int64_t{};
  if (!parse_timecode(to_utf8(ui->leChStart->text()), startTimecode))
    return { false, QY("The start time could not be parsed: %1").arg(Q(timecode_parser_error)) };
  GetChild<KaxChapterTimeStart>(*chapter).SetValue(startTimecode);

  if (!ui->leChEnd->text().isEmpty()) {
    auto endTimecode = int64_t{};
    if (!parse_timecode(to_utf8(ui->leChEnd->text()), endTimecode))
      return { false, QY("The end time could not be parsed: %1").arg(Q(timecode_parser_error)) };

    if (endTimecode <= startTimecode)
      return { false, QY("The end time must be greater than the start timecode.") };

    GetChild<KaxChapterTimeEnd>(*chapter).SetValue(endTimecode);

  } else
    DeleteChildren<KaxChapterTimeEnd>(*chapter);

  if (!ui->leChSegmentUid->text().isEmpty()) {
    try {
      auto value = bitvalue_c{to_utf8(ui->leChSegmentUid->text())};
      GetChild<KaxChapterSegmentUID>(*chapter).CopyBuffer(value.data(), value.byte_size());

    } catch (mtx::bitvalue_parser_x const &ex) {
      return { false, QY("The segment UID could not be parsed: %1").arg(ex.what()) };
    }

  } else
    DeleteChildren<KaxChapterSegmentUID>(*chapter);

  if (!ui->leChSegmentEditionUid->text().isEmpty()) {
    auto ok = false;
    uid     = ui->leChSegmentEditionUid->text().toULongLong(&ok);
    if (!ok || !uid)
      return { false, QY("The segment edition UID must be a positive number if given.") };

    GetChild<KaxChapterSegmentEditionUID>(*chapter).SetValue(uid);
  }

  RemoveChildren<KaxChapterDisplay>(*chapter);
  for (auto row = 0, numRows = m_nameModel->rowCount(); row < numRows; ++row)
    chapter->PushElement(*m_nameModel->displayFromIndex(m_nameModel->index(row, 0)));

  return { true, QString{} };
}

Tab::ValidationResult
Tab::copyEditionControlsToStorage(EditionPtr const &edition) {
  if (!edition)
    return { true, QString{} };

  auto uid = uint64_t{};

  if (!ui->leEdUid->text().isEmpty()) {
    auto ok = false;
    uid     = ui->leEdUid->text().toULongLong(&ok);
    if (!ok)
      return { false, QY("The edition UID must be a number if given.") };
  }

  if (uid)
    GetChild<KaxEditionUID>(*edition).SetValue(uid);
  else
    DeleteChildren<KaxEditionUID>(*edition);

  if (ui->cbEdFlagDefault->isChecked())
    GetChild<KaxEditionFlagDefault>(*edition).SetValue(1);
  else
    DeleteChildren<KaxEditionFlagDefault>(*edition);

  if (ui->cbEdFlagHidden->isChecked())
    GetChild<KaxEditionFlagHidden>(*edition).SetValue(1);
  else
    DeleteChildren<KaxEditionFlagHidden>(*edition);

  if (ui->cbEdFlagOrdered->isChecked())
    GetChild<KaxEditionFlagOrdered>(*edition).SetValue(1);
  else
    DeleteChildren<KaxEditionFlagOrdered>(*edition);

  return { true, QString{} };
}

bool
Tab::setControlsFromStorage() {
  auto idx = Util::selectedRowIdx(ui->elements);
  return idx.isValid() ? setControlsFromStorage(idx) : true;
}

bool
Tab::setControlsFromStorage(QModelIndex const &idx) {
  auto stdItem = m_chapterModel->itemFromIndex(idx);
  if (!stdItem)
    return false;

  if (!idx.parent().isValid())
    return setEditionControlsFromStorage(m_chapterModel->editionFromItem(stdItem));
  return setChapterControlsFromStorage(m_chapterModel->chapterFromItem(stdItem));
}

bool
Tab::setChapterControlsFromStorage(ChapterPtr const &chapter) {
  if (!chapter)
    return true;

  auto uid               = FindChildValue<KaxChapterUID>(*chapter);
  auto end               = FindChild<KaxChapterTimeEnd>(*chapter);
  auto segmentEditionUid = FindChild<KaxChapterSegmentEditionUID>(*chapter);

  ui->lChapter->setText(m_chapterModel->chapterDisplayName(*chapter));
  ui->leChStart->setText(Q(format_timecode(FindChildValue<KaxChapterTimeStart>(*chapter))));
  ui->leChEnd->setText(end ? Q(format_timecode(end->GetValue())) : Q(""));
  ui->cbChFlagEnabled->setChecked(!!FindChildValue<KaxChapterFlagEnabled>(*chapter, 1));
  ui->cbChFlagHidden->setChecked(!!FindChildValue<KaxChapterFlagHidden>(*chapter));
  ui->leChUid->setText(uid ? QString::number(uid) : Q(""));
  ui->leChSegmentUid->setText(formatEbmlBinary(FindChild<KaxChapterSegmentUID>(*chapter)));
  ui->leChSegmentEditionUid->setText(segmentEditionUid ? QString::number(segmentEditionUid->GetValue()) : Q(""));

  auto nameSelectionModel        = ui->tvChNames->selectionModel();
  auto previouslySelectedNameIdx = nameSelectionModel->currentIndex();
  m_nameModel->populate(*chapter);
  enableNameWidgets(false);

  if (m_nameModel->rowCount()) {
    auto oldBlocked  = nameSelectionModel->blockSignals(true);
    auto rowToSelect = std::min(previouslySelectedNameIdx.isValid() ? previouslySelectedNameIdx.row() : 0, m_nameModel->rowCount());
    auto selection   = QItemSelection{ m_nameModel->index(rowToSelect, 0), m_nameModel->index(rowToSelect, m_nameModel->columnCount() - 1) };

    nameSelectionModel->select(selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Current);

    setNameControlsFromStorage(m_nameModel->index(rowToSelect, 0));
    enableNameWidgets(true);

    nameSelectionModel->blockSignals(oldBlocked);
  }

  ui->pageContainer->setCurrentWidget(ui->chapterPage);

  return true;
}

bool
Tab::setEditionControlsFromStorage(EditionPtr const &edition) {
  if (!edition)
    return true;

  auto uid = FindChildValue<KaxEditionUID>(*edition);

  ui->leEdUid->setText(uid ? QString::number(uid) : Q(""));
  ui->cbEdFlagDefault->setChecked(!!FindChildValue<KaxEditionFlagDefault>(*edition));
  ui->cbEdFlagHidden->setChecked(!!FindChildValue<KaxEditionFlagHidden>(*edition));
  ui->cbEdFlagOrdered->setChecked(!!FindChildValue<KaxEditionFlagOrdered>(*edition));

  ui->pageContainer->setCurrentWidget(ui->editionPage);

  return true;
}

bool
Tab::handleChapterDeselection(QItemSelection const &deselected) {
  if (deselected.isEmpty())
    return true;

  auto indexes = deselected.at(0).indexes();
  return indexes.isEmpty() ? true : copyControlsToStorage(indexes.at(0));
}

void
Tab::chapterSelectionChanged(QItemSelection const &selected,
                             QItemSelection const &deselected) {
  auto selectedIdx = QModelIndex{};
  if (!selected.isEmpty()) {
    auto indexes = selected.at(0).indexes();
    if (!indexes.isEmpty())
      selectedIdx = indexes.at(0);
  }

  m_chapterModel->setSelectedIdx(selectedIdx);

  if (m_ignoreChapterSelectionChanges)
    return;

  if (!handleChapterDeselection(deselected))
    return;

  if (selectedIdx.isValid() && setControlsFromStorage(selectedIdx.sibling(selectedIdx.row(), 0)))
    return;

  ui->pageContainer->setCurrentWidget(ui->emptyPage);
}

bool
Tab::setNameControlsFromStorage(QModelIndex const &idx) {
  auto display = m_nameModel->displayFromIndex(idx);
  if (!display)
    return false;

  ui->leChName->setText(Q(GetChildValue<KaxChapterString>(display)));
  Util::setComboBoxTextByData(ui->cbChNameLanguage, Q(FindChildValue<KaxChapterLanguage>(display, std::string{"eng"})));
  Util::setComboBoxTextByData(ui->cbChNameCountry,  Q(FindChildValue<KaxChapterCountry>(display)));

  resizeNameColumnsToContents();

  return true;
}

void
Tab::nameSelectionChanged(QItemSelection const &selected,
                          QItemSelection const &) {
  if (!selected.isEmpty()) {
    auto indexes = selected.at(0).indexes();
    if (!indexes.isEmpty() && setNameControlsFromStorage(indexes.at(0))) {
      enableNameWidgets(true);

      ui->leChName->selectAll();
      QTimer::singleShot(0, ui->leChName, SLOT(setFocus()));

      return;
    }
  }

  enableNameWidgets(false);
}

void
Tab::withSelectedName(std::function<void(QModelIndex const &, KaxChapterDisplay &)> const &worker) {
  auto selectedRows = ui->tvChNames->selectionModel()->selectedRows();
  if (selectedRows.isEmpty())
    return;

  auto idx     = selectedRows.at(0);
  auto display = m_nameModel->displayFromIndex(idx);
  if (display)
    worker(idx, *display);
}

void
Tab::chapterNameEdited(QString const &text) {
  withSelectedName([this, &text](QModelIndex const &idx, KaxChapterDisplay &display) {
    GetChild<KaxChapterString>(display).SetValueUTF8(to_utf8(text));
    m_nameModel->updateRow(idx.row());
  });
}

void
Tab::chapterNameLanguageChanged(int index) {
  if (0 > index)
    return;

  withSelectedName([this, index](QModelIndex const &idx, KaxChapterDisplay &display) {
    GetChild<KaxChapterLanguage>(display).SetValue(to_utf8(ui->cbChNameLanguage->itemData(index).toString()));
    m_nameModel->updateRow(idx.row());
  });
}

void
Tab::chapterNameCountryChanged(int index) {
  if (0 > index)
    return;

  withSelectedName([this, index](QModelIndex const &idx, KaxChapterDisplay &display) {
    if (0 == index)
      DeleteChildren<KaxChapterCountry>(display);
    else
      GetChild<KaxChapterCountry>(display).SetValue(to_utf8(ui->cbChNameCountry->currentData().toString()));
    m_nameModel->updateRow(idx.row());
  });
}

void
Tab::addChapterName() {
  m_nameModel->addNew();
}

void
Tab::removeChapterName() {
  auto idx = Util::selectedRowIdx(ui->tvChNames);
  if (!idx.isValid())
    return;

  m_nameModel->remove(idx);
}

void
Tab::enableNameWidgets(bool enable) {
  for (auto const &widget : m_nameWidgets)
    widget->setEnabled(enable);
}

void
Tab::expandAll() {
  expandCollapseAll(true);
}

void
Tab::collapseAll() {
  expandCollapseAll(false);
}

void
Tab::addEditionBefore() {
  addEdition(true);
}

void
Tab::addEditionAfter() {
  addEdition(false);
}

void
Tab::addEdition(bool before) {
  auto edition     = std::make_shared<KaxEditionEntry>();
  auto selectedIdx = Util::selectedRowIdx(ui->elements);
  auto row         = 0;

  if (selectedIdx.isValid()) {
    while (selectedIdx.parent().isValid())
      selectedIdx = selectedIdx.parent();

    row = selectedIdx.row() + (before ? 0 : 1);
  }

  GetChild<KaxEditionUID>(*edition).SetValue(0);

  m_chapterModel->insertEdition(row, edition);

  emit numberOfEntriesChanged();
}

ChapterPtr
Tab::createEmptyChapter(int64_t startTime,
                        int chapterNumber) {
  auto &cfg     = Util::Settings::get();
  auto chapter  = std::make_shared<KaxChapterAtom>();
  auto &display = GetChild<KaxChapterDisplay>(*chapter);
  auto name     = QString{ cfg.m_chapterNameTemplate };

  name.replace(Q("<NUM>"), QString::number(chapterNumber));

  GetChild<KaxChapterUID>(*chapter).SetValue(0);
  GetChild<KaxChapterTimeStart>(*chapter).SetValue(startTime);
  GetChild<KaxChapterString>(display).SetValue(to_wide(name));
  GetChild<KaxChapterLanguage>(display).SetValue(to_utf8(cfg.m_defaultChapterLanguage));
  if (!cfg.m_defaultChapterCountry.isEmpty())
    GetChild<KaxChapterCountry>(display).SetValue(to_utf8(cfg.m_defaultChapterCountry));

  return chapter;
}

void
Tab::addChapterBefore() {
  addChapter(true);
}

void
Tab::addChapterAfter() {
  addChapter(false);
}

void
Tab::addChapter(bool before) {
  auto selectedIdx = Util::selectedRowIdx(ui->elements);
  if (!selectedIdx.isValid() || !selectedIdx.parent().isValid())
    return;

  // TODO: Tab::addChapter: start time
  auto row     = selectedIdx.row() + (before ? 0 : 1);
  auto chapter = createEmptyChapter(0, row + 1);

  m_chapterModel->insertChapter(row, chapter, selectedIdx.parent());

  emit numberOfEntriesChanged();
}

void
Tab::addSubChapter() {
  auto selectedIdx = Util::selectedRowIdx(ui->elements);
  if (!selectedIdx.isValid())
    return;

  // TODO: Tab::addSubChapter: start time
  auto selectedItem = m_chapterModel->itemFromIndex(selectedIdx);
  auto chapter      = createEmptyChapter(0, (selectedItem ? selectedItem->rowCount() : 0) + 1);

  m_chapterModel->appendChapter(chapter, selectedIdx);
  expandCollapseAll(true, selectedIdx);

  emit numberOfEntriesChanged();
}

void
Tab::removeElement() {
  m_chapterModel->removeTree(Util::selectedRowIdx(ui->elements));
  emit numberOfEntriesChanged();
}

void
Tab::shiftTimecodes(QStandardItem *item,
                    int64_t delta) {
  if (!item)
    return;

  if (item->parent()) {
    auto chapter = m_chapterModel->chapterFromItem(item);
    if (chapter) {
      auto kStart = FindChild<KaxChapterTimeStart>(*chapter);
      auto kEnd   = FindChild<KaxChapterTimeEnd>(*chapter);

      if (kStart)
        kStart->SetValue(std::max<int64_t>(static_cast<int64_t>(kStart->GetValue()) + delta, 0));
      if (kEnd)
        kEnd->SetValue(std::max<int64_t>(static_cast<int64_t>(kEnd->GetValue()) + delta, 0));

      if (kStart || kEnd)
        m_chapterModel->updateRow(item->index());
    }
  }

  for (auto row = 0, numRows = item->rowCount(); row < numRows; ++row)
    shiftTimecodes(item->child(row), delta);
}

void
Tab::constrictTimecodes(QStandardItem *item,
                        boost::optional<uint64_t> const &constrictStart,
                        boost::optional<uint64_t> const &constrictEnd) {
  if (!item)
    return;

  auto chapter = item->parent() ? m_chapterModel->chapterFromItem(item) : ChapterPtr{};
  if (!chapter) {
    for (auto row = 0, numRows = item->rowCount(); row < numRows; ++row)
      constrictTimecodes(item->child(row), {}, {});
  return;
  }

  auto kStart   = &GetChild<KaxChapterTimeStart>(*chapter);
  auto kEnd     = FindChild<KaxChapterTimeEnd>(*chapter);
  auto newStart = !constrictStart ? kStart->GetValue()
                : !constrictEnd   ? std::max(*constrictStart, kStart->GetValue())
                :                   std::min(*constrictEnd, std::max(*constrictStart, kStart->GetValue()));
  auto newEnd   = !kEnd           ? boost::optional<uint64_t>{}
                : !constrictEnd   ? std::max(newStart, kEnd->GetValue())
                :                   std::max(newStart, std::min(*constrictEnd, kEnd->GetValue()));

  kStart->SetValue(newStart);
  if (newEnd)
    GetChild<KaxChapterTimeEnd>(*chapter).SetValue(*newEnd);

  m_chapterModel->updateRow(item->index());

  for (auto row = 0, numRows = item->rowCount(); row < numRows; ++row)
    constrictTimecodes(item->child(row), newStart, newEnd);
}

std::pair<boost::optional<uint64_t>, boost::optional<uint64_t>>
Tab::expandTimecodes(QStandardItem *item) {
  if (!item)
    return {};

  auto chapter = m_chapterModel->chapterFromItem(item);
  if (!chapter) {
    for (auto row = 0, numRows = item->rowCount(); row < numRows; ++row)
      expandTimecodes(item->child(row));
    return {};
  }

  auto kStart   = chapter ? FindChild<KaxChapterTimeStart>(*chapter)      : nullptr;
  auto kEnd     = chapter ? FindChild<KaxChapterTimeEnd>(*chapter)        : nullptr;
  auto newStart = kStart  ? boost::optional<uint64_t>{kStart->GetValue()} : boost::optional<uint64_t>{};
  auto newEnd   = kEnd    ? boost::optional<uint64_t>{kEnd->GetValue()}   : boost::optional<uint64_t>{};
  auto modified = false;

  for (auto row = 0, numRows = item->rowCount(); row < numRows; ++row) {
    auto startAndEnd = expandTimecodes(item->child(row));

    if (!newStart || (startAndEnd.first && (*startAndEnd.first < *newStart)))
      newStart = startAndEnd.first;

    if (!newEnd || (startAndEnd.second && (*startAndEnd.second > *newEnd)))
      newEnd = startAndEnd.second;
  }

  if (newStart && (!kStart || (kStart->GetValue() > *newStart))) {
    GetChild<KaxChapterTimeStart>(*chapter).SetValue(*newStart);
    modified = true;
  }

  if (newEnd && (!kEnd || (kEnd->GetValue() < *newEnd))) {
    GetChild<KaxChapterTimeEnd>(*chapter).SetValue(*newEnd);
    modified = true;
  }

  if (modified)
    m_chapterModel->updateRow(item->index());

  return std::make_pair(newStart, newEnd);
}

void
Tab::setLanguages(QStandardItem *item,
                  QString const &language) {
  if (!item)
    return;

  auto chapter = m_chapterModel->chapterFromItem(item);
  if (chapter)
    for (auto const &element : *chapter) {
      auto kDisplay = dynamic_cast<KaxChapterDisplay *>(element);
      if (kDisplay)
        GetChild<KaxChapterLanguage>(*kDisplay).SetValue(to_utf8(language));
    }

  for (auto row = 0, numRows = item->rowCount(); row < numRows; ++row)
    setLanguages(item->child(row), language);
}

void
Tab::setCountries(QStandardItem *item,
                  QString const &country) {
  if (!item)
    return;

  auto chapter = m_chapterModel->chapterFromItem(item);
  if (chapter)
    for (auto const &element : *chapter) {
      auto kDisplay = dynamic_cast<KaxChapterDisplay *>(element);
      if (!kDisplay)
        continue;

      if (country.isEmpty())
        DeleteChildren<KaxChapterCountry>(*kDisplay);
      else
        GetChild<KaxChapterCountry>(*kDisplay).SetValue(to_utf8(country));
    }

  for (auto row = 0, numRows = item->rowCount(); row < numRows; ++row)
    setCountries(item->child(row), country);
}

void
Tab::massModify() {
  if (!copyControlsToStorage())
    return;

  auto selectedIdx = Util::selectedRowIdx(ui->elements);
  MassModificationDialog dlg{this, selectedIdx.isValid()};
  if (!dlg.exec())
    return;

  auto actions = dlg.actions();
  auto item    = selectedIdx.isValid() ? m_chapterModel->itemFromIndex(selectedIdx) : m_chapterModel->invisibleRootItem();

  if (actions & MassModificationDialog::Shift)
    shiftTimecodes(item, dlg.shiftBy());

  if (actions & MassModificationDialog::Constrict)
    constrictTimecodes(item, {}, {});

  if (actions & MassModificationDialog::Expand)
    expandTimecodes(item);

  if (actions & MassModificationDialog::SetLanguage)
    setLanguages(item, dlg.language());

  if (actions & MassModificationDialog::SetCountry)
    setCountries(item, dlg.country());

  if (actions & MassModificationDialog::Sort)
    item->sortChildren(1);

  setControlsFromStorage();
}

void
Tab::duplicateElement() {
  auto selectedIdx   = Util::selectedRowIdx(ui->elements);
  auto newElementIdx = m_chapterModel->duplicateTree(selectedIdx);

  if (newElementIdx.isValid())
    expandCollapseAll(true, newElementIdx);

  emit numberOfEntriesChanged();
}

void
Tab::expandCollapseAll(bool expand,
                       QModelIndex const &parentIdx) {
  if (parentIdx.isValid())
    ui->elements->setExpanded(parentIdx, expand);

  for (auto row = 0, numRows = m_chapterModel->rowCount(parentIdx); row < numRows; ++row)
    expandCollapseAll(expand, m_chapterModel->index(row, 0, parentIdx));
}

void
Tab::expandInsertedElements(QModelIndex const &parentIdx,
                            int,
                            int) {
  expandCollapseAll(true, parentIdx);
}

QString
Tab::formatEbmlBinary(EbmlBinary *binary) {
  auto value = std::string{};
  auto data  = static_cast<unsigned char const *>(binary ? binary->GetBuffer() : nullptr);

  if (data)
    for (auto end = data + binary->GetSize(); data < end; ++data)
      value += (boost::format("%|1$02x|") % static_cast<unsigned int>(*data)).str();

  return Q(value);
}

void
Tab::showChapterContextMenu(QPoint const &pos) {
  auto selectedIdx     = Util::selectedRowIdx(ui->elements);
  auto hasSelection    = selectedIdx.isValid();
  auto chapterSelected = hasSelection && selectedIdx.parent().isValid();
  auto hasEntries      = !!m_chapterModel->rowCount();

  m_addChapterBeforeAction->setEnabled(chapterSelected);
  m_addChapterAfterAction->setEnabled(chapterSelected);
  m_addSubChapterAction->setEnabled(hasSelection);
  m_removeElementAction->setEnabled(hasSelection);
  m_duplicateAction->setEnabled(hasSelection);
  m_expandAllAction->setEnabled(hasEntries);
  m_collapseAllAction->setEnabled(hasEntries);

  QMenu menu{this};

  menu.addAction(m_addEditionBeforeAction);
  menu.addAction(m_addEditionAfterAction);
  menu.addSeparator();
  menu.addAction(m_addChapterBeforeAction);
  menu.addAction(m_addChapterAfterAction);
  menu.addAction(m_addSubChapterAction);
  menu.addSeparator();
  menu.addAction(m_duplicateAction);
  menu.addSeparator();
  menu.addAction(m_removeElementAction);
  menu.addSeparator();
  menu.addAction(m_massModificationAction);
  menu.addSeparator();
  menu.addAction(m_expandAllAction);
  menu.addAction(m_collapseAllAction);

  menu.exec(ui->elements->viewport()->mapToGlobal(pos));
}

bool
Tab::hasChapters()
  const {
  for (auto idx = 0, numEditions = m_chapterModel->rowCount(); idx < numEditions; ++idx)
    if (m_chapterModel->item(idx)->rowCount())
      return true;
  return false;
}

QString
Tab::currentState()
  const {
  auto chapters = m_chapterModel->allChapters();
  return chapters ? Q(ebml_dumper_c::dump_to_string(chapters.get(), static_cast<ebml_dumper_c::dump_style_e>(ebml_dumper_c::style_with_values | ebml_dumper_c::style_with_indexes))) : QString{};
}

bool
Tab::hasBeenModified()
  const {
  return currentState() != m_savedState;
}

}}}
