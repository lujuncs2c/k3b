#include "k3bdeviceoptiontab.h"
#include "../device/k3bdevicemanager.h"
#include "../device/k3bdevicewidget.h"
#include "../tools/k3bglobals.h"

#include <qlabel.h>
#include <qstring.h>
#include <qlayout.h>

#include <kapplication.h>
#include <kdialog.h>
#include <klocale.h>
#include <kconfig.h>
#include <kstandarddirs.h>


K3bDeviceOptionTab::K3bDeviceOptionTab( QWidget* parent, const char* name )
  : QWidget( parent, name )
{
  QGridLayout* frameLayout = new QGridLayout( this );
  frameLayout->setSpacing( KDialog::spacingHint() );
  frameLayout->setMargin( 0 );


  // Info Label
  // ------------------------------------------------
  m_labelDevicesInfo = new QLabel( this, "m_labelDevicesInfo" );
  m_labelDevicesInfo->setAlignment( int( QLabel::WordBreak | QLabel::AlignVCenter | QLabel::AlignLeft ) );
  m_labelDevicesInfo->setText( i18n( "K3b tries to detect all your devices properly. "
				     "You can add devices that have not been detected and change "
				     "the black values by clicking in the list. If K3b is unable "
				     "to detect your drive, run K3bSetup to set the correct permissions." ) );
  // ------------------------------------------------

  m_deviceWidget = new K3bDeviceWidget( K3bDeviceManager::self(), this );

  frameLayout->addWidget( m_labelDevicesInfo, 0, 0 );
  frameLayout->addWidget( m_deviceWidget, 1, 0 );

  connect( m_deviceWidget, SIGNAL(refreshButtonClicked()), this, SLOT(slotRefreshButtonClicked()) );
}


K3bDeviceOptionTab::~K3bDeviceOptionTab()
{
}


void K3bDeviceOptionTab::readDevices()
{
  m_deviceWidget->init();
}



void K3bDeviceOptionTab::saveDevices()
{
  // save changes to deviceManager
  m_deviceWidget->apply();

  // save the config
  K3bDeviceManager::self()->saveConfig( kapp->config() );
}


void K3bDeviceOptionTab::slotRefreshButtonClicked()
{
  K3bDeviceManager::self()->clear();
  K3bDeviceManager::self()->scanbus();
  
  KConfig globalConfig( K3b::globalConfig() );
  
  globalConfig.setGroup( "Devices" );
  K3bDeviceManager::self()->readConfig( &globalConfig );

  m_deviceWidget->init();
}

#include "k3bdeviceoptiontab.moc"
