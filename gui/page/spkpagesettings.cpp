
#include "spkutils.h"
#include "page/spkpagesettings.h"

namespace SpkUi
{
  SpkPageSettings::SpkPageSettings(QWidget *parent) :
    SpkPageBase(parent)
  {
    mMainArea = new QScrollArea();
    mMainLay = new QVBoxLayout(this);
    mSettingsWidget = new QWidget(this);
    mSettingsUi = new Ui::SpkUiSettings;

    mSettingsUi->setupUi(mSettingsWidget);

    mMainLay->addWidget(mMainArea);

    mMainArea->setWidget(mSettingsWidget);
    mMainArea->setWidgetResizable(true);

    SetupUi();
  }

  SpkPageSettings::~SpkPageSettings()
  {
    delete mSettingsWidget;
  }

  void SpkPageSettings::SetupUi()
  {
    auto ui = mSettingsUi;

    ui->lblSettingsTitle->setObjectName("styConfTitle");
    ui->lblCleanup->setObjectName("styConfTitle");
    ui->lblAdvanced->setObjectName("styConfTitle");

    connect(ui->btnSave, &QPushButton::clicked,
            this, &SpkPageSettings::SaveConfiguration);
  }

  void SpkPageSettings::ReadConfiguration()
  {
    auto ui = mSettingsUi;

    ui->spnConcurrentResDownloads->setValue(CFG->ReadField("resource/concurrent", 5).toInt());
    ui->edtApiUrl->setText(CFG->ReadField("url/api", "").toString());
    ui->edtResourceUrl->setText(CFG->ReadField("url/res", "").toString());
    ui->edtResourceCachePath->setText(CFG->ReadField("dirs/cache", "").toString());
    ui->edtDownloadPath->setText(CFG->ReadField("dirs/download", "").toString());
    ui->edtDownloadServers->setPlainText(CFG->ReadField("download/servers", "").toString());
    ui->edtQssPath->setText(CFG->ReadField("internal/qss_path", "").toString());
  }

  void SpkPageSettings::SaveConfiguration()
  {
    auto ui = mSettingsUi;
    auto settings = CFG;

    CFG->SetSettings("resource/concurrent", ui->spnConcurrentResDownloads->value());
    assert(CFG->SetField("url/api", ui->edtApiUrl->text()));
    assert(CFG->SetField("url/res", ui->edtResourceUrl->text()));
    CFG->SetSettings("dirs/cache", ui->edtResourceCachePath->text());
    assert(CFG->SetField("dirs/download", ui->edtDownloadPath->text()));
    assert(CFG->SetField("download/servers", ui->edtDownloadServers->toPlainText()));
    CFG->SetSettings("internal/qss_path", ui->edtQssPath->text());
  }

  void SpkPageSettings::Activated()
  {
    ReadConfiguration();
  }
}