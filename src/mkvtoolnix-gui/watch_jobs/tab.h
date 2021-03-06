#ifndef MTX_MKVTOOLNIX_GUI_WATCH_JOBS_TAB_H
#define MTX_MKVTOOLNIX_GUI_WATCH_JOBS_TAB_H

#include "common/common_pch.h"

#include <QWidget>

#include "mkvtoolnix-gui/jobs/job.h"

class QDateTime;
class QLabel;

namespace mtx { namespace gui { namespace WatchJobs {

namespace Ui {
class Tab;
}

class Tab : public QWidget {
  Q_OBJECT;

protected:
  // UI stuff:
  std::unique_ptr<Ui::Tab> ui;
  QStringList m_fullOutput;
  uint64_t m_id, m_currentJobProgress, m_queueProgress;
  Jobs::Job::Status m_currentJobStatus;
  QDateTime m_currentJobStartTime;

  // Only use this variable for determining whether or not to ignore
  // certain signals.
  QObject const *m_currentlyConnectedJob;

public:
  explicit Tab(QWidget *parent);
  ~Tab();

  virtual void retranslateUi();

  virtual void connectToJob(mtx::gui::Jobs::Job const &job);
  virtual void setInitialDisplay(mtx::gui::Jobs::Job const &job);

  uint64_t id() const;

  bool isSaveOutputEnabled() const;
  bool isCurrentJobTab() const;

signals:
  void abortJob();

public slots:
  void onStatusChanged(uint64_t id);
  void onJobProgressChanged(uint64_t id, unsigned int progress);
  void onQueueProgressChanged(int progress, int totalProgress);
  void onLineRead(QString const &line, mtx::gui::Jobs::Job::LineType type);
  void onAbort();

  void onSaveOutput();

  void acknowledgeWarningsAndErrors();

  void updateRemainingTime();

protected:
  void updateOneRemainingTimeLabel(QLabel *label, QDateTime const &startTime, uint64_t progress);
};

}}}

#endif // MTX_MKVTOOLNIX_GUI_WATCH_JOBS_TAB_H
