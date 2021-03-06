#ifndef MTX_MKVTOOLNIX_GUI_HEADER_EDITOR_TOOL_H
#define MTX_MKVTOOLNIX_GUI_HEADER_EDITOR_TOOL_H

#include "common/common_pch.h"

#include "mkvtoolnix-gui/main_window/tool_base.h"
#include "mkvtoolnix-gui/util/files_drag_drop_handler.h"

class QDragEnterEvent;
class QDropEvent;
class QMenu;

namespace mtx { namespace gui { namespace HeaderEditor {

namespace Ui {
class Tool;
}

class Tab;

class Tool : public ToolBase {
  Q_OBJECT;

protected:
  // UI stuff:
  std::unique_ptr<Ui::Tool> ui;
  QMenu *m_headerEditorMenu;
  mtx::gui::Util::FilesDragDropHandler m_filesDDHandler;

public:
  explicit Tool(QWidget *parent, QMenu *headerEditorMenu);
  ~Tool();

  virtual void retranslateUi() override;

  virtual void dragEnterEvent(QDragEnterEvent *event) override;
  virtual void dropEvent(QDropEvent *event) override;

public slots:
  virtual void toolShown() override;
  virtual void selectFileToOpen();
  virtual void save();
  virtual void validate();
  virtual bool closeTab(int index);
  virtual void closeCurrentTab();
  virtual void closeSendingTab();
  virtual bool closeAllTabs();
  virtual void reload();
  virtual void openFiles(QStringList const &fileNames);
  virtual void openFilesFromCommandLine(QStringList const &fileNames);

protected:
  virtual void openFile(QString const &fileName);
  virtual void setupActions();
  virtual void showHeaderEditorsWidget();
  virtual Tab *currentTab();
};

}}}

#endif // MTX_MKVTOOLNIX_GUI_HEADER_EDITOR_TOOL_H
