/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bburningoptiontab.h"
#include "../k3b.h"

#include <qlabel.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qgroupbox.h>
#include <qtabwidget.h>
#include <qradiobutton.h>
#include <qvalidator.h>
#include <qbuttongroup.h>
#include <qspinbox.h>
#include <qtooltip.h>
#include <qwhatsthis.h>

#include <knuminput.h>
#include <kconfig.h>
#include <kdialog.h>
#include <klocale.h>
#include <klineedit.h>


K3bBurningOptionTab::K3bBurningOptionTab( QWidget* parent, const char* name )
  : QWidget( parent, name )
{
  setupGui();

  m_bPregapSeconds = false;
  m_comboPregapFormat->setCurrentItem( 1 );
}


K3bBurningOptionTab::~K3bBurningOptionTab()
{
}


void K3bBurningOptionTab::setupGui()
{
  QVBoxLayout* box = new QVBoxLayout( this );
  box->setAutoAdd( true );

  QTabWidget* mainTabbed = new QTabWidget( this );


  // PROJECT TAB
  // //////////////////////////////////////////////////////////////////////
  QWidget* projectTab = new QWidget( mainTabbed );

  // audio settings group
  // -----------------------------------------------------------------------
  QGroupBox* m_groupAudio = new QGroupBox( projectTab, "m_groupAudio" );
  m_groupAudio->setTitle( i18n( "Audio Project" ) );
  m_groupAudio->setColumnLayout(0, Qt::Vertical );
  m_groupAudio->layout()->setSpacing( 0 );
  m_groupAudio->layout()->setMargin( 0 );
  QGridLayout* groupAudioLayout = new QGridLayout( m_groupAudio->layout() );
  groupAudioLayout->setAlignment( Qt::AlignTop );
  groupAudioLayout->setSpacing( KDialog::spacingHint() );
  groupAudioLayout->setMargin( KDialog::marginHint() );

  m_editDefaultPregap = new KIntNumInput( m_groupAudio );
  m_comboPregapFormat = new QComboBox( m_groupAudio );

  QLabel* labelDefaultPregap = new QLabel( i18n("&Default pregap:"), m_groupAudio );
  labelDefaultPregap->setBuddy( m_editDefaultPregap );

  groupAudioLayout->addWidget( labelDefaultPregap, 0, 0 );
  groupAudioLayout->addWidget( m_editDefaultPregap, 0, 1 );
  groupAudioLayout->addWidget( m_comboPregapFormat, 0, 2 );

  m_comboPregapFormat->insertItem( i18n( "Seconds" ) );
  m_comboPregapFormat->insertItem( i18n( "Frames" ) );

  connect( m_comboPregapFormat, SIGNAL(activated(const QString&)),
	   this, SLOT(slotChangePregapFormat(const QString&)) );
  // -----------------------------------------------------------------------


  // data settings group
  // -----------------------------------------------------------------------
  QGroupBox* m_groupData = new QGroupBox( 2, Qt::Vertical, i18n( "Data Project" ), projectTab, "m_groupData" );
  m_groupData->layout()->setSpacing( KDialog::spacingHint() );
  m_groupData->layout()->setMargin( KDialog::marginHint() );

  m_checkUseID3Tag = new QCheckBox( i18n("&Use audio tags for filenames"), m_groupData );
  m_checkDropDoubles = new QCheckBox( i18n("&Discard identical names"), m_groupData );
  m_checkListHiddenFiles = new QCheckBox( i18n("List &hidden files"), m_groupData );
  m_checkListSystemFiles = new QCheckBox( i18n("List &system files"), m_groupData );

  // -----------------------------------------------------------------------
  // vcd settings group
  // -----------------------------------------------------------------------
  QGroupBox* groupVideo = new QGroupBox( projectTab, "groupVideo" );
  groupVideo->setTitle( i18n( "Video Project" ) );
  groupVideo->setColumnLayout(0, Qt::Vertical );
  groupVideo->layout()->setSpacing( 0 );
  groupVideo->layout()->setMargin( 0 );
  QGridLayout* groupVideoLayout = new QGridLayout( groupVideo->layout() );
  groupVideoLayout->setAlignment( Qt::AlignTop );
  groupVideoLayout->setSpacing( KDialog::spacingHint() );
  groupVideoLayout->setMargin( KDialog::marginHint() );

  m_checkUsePbc = new QCheckBox( i18n("Use playback control (PBC) by default"), groupVideo );
  m_labelPlayTime = new QLabel( i18n("Play each sequence/segment by default"), groupVideo );
  m_spinPlayTime = new QSpinBox( groupVideo, "m_spinPlayTime" );
  m_spinPlayTime->setValue( 1 );
  m_spinPlayTime->setSuffix( i18n( " time(s)" ) );
  m_spinPlayTime->setSpecialValueText( i18n( "forever" ) );  

  m_labelWaitTime = new QLabel( i18n("Time to wait after each sequence/segment by default"), groupVideo );
  m_spinWaitTime = new QSpinBox( groupVideo, "m_spinWaitTime" );
  m_spinWaitTime->setMinValue( -1 );
  m_spinWaitTime->setValue( 2 );
  m_spinWaitTime->setSuffix( i18n( " second(s)" ) );
  m_spinWaitTime->setSpecialValueText( i18n( "infinite" ) );  

  /* not implemented yet ********************************/
  m_checkUseNumKey = new QCheckBox( i18n("Use numeric keys by default"), groupVideo );
  m_checkUseNumKey->setHidden( true );
  /*************************************************/
  
  m_labelPlayTime->setDisabled( true );
  m_spinPlayTime->setDisabled( true );
  m_labelWaitTime->setDisabled( true );
  m_spinWaitTime->setDisabled( true );
  m_checkUseNumKey->setDisabled( true );

  groupVideoLayout->addMultiCellWidget( m_checkUsePbc, 0, 0, 0, 1 );
  groupVideoLayout->addMultiCellWidget( m_checkUseNumKey, 1, 1, 0, 1 );
  groupVideoLayout->addWidget( m_labelPlayTime, 2, 0 );
  groupVideoLayout->addWidget( m_spinPlayTime, 2, 1 );
  groupVideoLayout->addWidget( m_labelWaitTime, 3, 0 );
  groupVideoLayout->addWidget( m_spinWaitTime, 3, 1 );


  connect( m_checkUsePbc, SIGNAL(toggled(bool)), m_labelPlayTime, SLOT(setEnabled(bool)) );
  connect( m_checkUsePbc, SIGNAL(toggled(bool)), m_spinPlayTime, SLOT(setEnabled(bool)) );
  connect( m_checkUsePbc, SIGNAL(toggled(bool)), m_labelWaitTime, SLOT(setEnabled(bool)) );
  connect( m_checkUsePbc, SIGNAL(toggled(bool)), m_spinWaitTime, SLOT(setEnabled(bool)) );
  connect( m_checkUsePbc, SIGNAL(toggled(bool)), m_checkUseNumKey, SLOT(setEnabled(bool)) );

  // -----------------------------------------------------------------------
  // default cd size group
  // -----------------------------------------------------------------------
  QButtonGroup* groupCdSize = new QButtonGroup( 0, Qt::Vertical, i18n("Default CD Size"), projectTab );
  groupCdSize->layout()->setSpacing( 0 );
  groupCdSize->layout()->setMargin( 0 );
  QGridLayout* groupCdSizeLayout = new QGridLayout( groupCdSize->layout() );
  groupCdSizeLayout->setAlignment( Qt::AlignTop );
  groupCdSizeLayout->setSpacing( KDialog::spacingHint() );
  groupCdSizeLayout->setMargin( KDialog::marginHint() );

  m_radio74Minutes    = new QRadioButton( i18n("&%1 minutes (%2 MB)").arg(74).arg(650), groupCdSize );
  m_radio80Minutes    = new QRadioButton( i18n("&%1 minutes (%2 MB)").arg(80).arg(700), groupCdSize );
  m_radio100Minutes   = new QRadioButton( i18n("&%1 minutes (%2 MB)").arg(90).arg(800), groupCdSize );
  m_radioCustomCdSize = new QRadioButton( i18n("&Custom:"), groupCdSize );
  m_editCustomCdSize  = new KLineEdit( groupCdSize );

  m_editCustomCdSize->setValidator( new QIntValidator( m_editCustomCdSize ) );

  groupCdSizeLayout->addMultiCellWidget( m_radio74Minutes, 0, 0, 0, 2 );
  groupCdSizeLayout->addMultiCellWidget( m_radio80Minutes, 1, 1, 0, 2 );
  groupCdSizeLayout->addMultiCellWidget( m_radio100Minutes, 2, 2, 0, 2 );
  groupCdSizeLayout->addWidget( m_radioCustomCdSize, 3, 0 );
  groupCdSizeLayout->addWidget( m_editCustomCdSize, 3, 1 );
  groupCdSizeLayout->addWidget( new QLabel( i18n("minutes"), groupCdSize ), 3, 2 );

  connect( m_radioCustomCdSize, SIGNAL(toggled(bool)), m_editCustomCdSize, SLOT(setEnabled(bool)) );
  connect( m_radioCustomCdSize, SIGNAL(toggled(bool)), m_editCustomCdSize, SLOT(setFocus()) );
  m_editCustomCdSize->setDisabled( true );
  // -----------------------------------------------------------------------


  QGridLayout* projectGrid = new QGridLayout( projectTab );
  projectGrid->setSpacing( KDialog::spacingHint() );
  projectGrid->setMargin( KDialog::marginHint() );

  projectGrid->addWidget( m_groupAudio, 0, 0 );
  projectGrid->addWidget( m_groupData, 1, 0 );
  projectGrid->addWidget( groupVideo, 2, 0 );
  projectGrid->addWidget( groupCdSize, 3, 0 );
  projectGrid->setRowStretch( 4, 1 );

  // ///////////////////////////////////////////////////////////////////////



  // advanced settings tab
  // -----------------------------------------------------------------------
  QWidget* advancedTab = new QWidget( mainTabbed );
  QGridLayout* groupAdvancedLayout = new QGridLayout( advancedTab );
  groupAdvancedLayout->setAlignment( Qt::AlignTop );
  groupAdvancedLayout->setSpacing( KDialog::spacingHint() );
  groupAdvancedLayout->setMargin( KDialog::marginHint() );



  QGroupBox* groupWritingApp = new QGroupBox( 0, Qt::Vertical, i18n("Writing Applications"), advancedTab );
  groupWritingApp->layout()->setMargin( 0 );
  QGridLayout* bufferLayout = new QGridLayout( groupWritingApp->layout() );
  bufferLayout->setMargin( KDialog::marginHint() );
  bufferLayout->setSpacing( KDialog::spacingHint() );

  m_checkOverburn = new QCheckBox( i18n("Allow &overburning (not supported by cdrecord <= 1.10)"), groupWritingApp );
  m_checkAllowWritingAppSelection = new QCheckBox( i18n("Manual writing application &selection"), groupWritingApp );
  m_checkManualWritingBufferSize = new QCheckBox( i18n("&Manual writing buffer size"), groupWritingApp );
  m_editWritingBufferSizeCdrecord = new KIntNumInput( 4, groupWritingApp );
  m_editWritingBufferSizeCdrdao = new KIntNumInput( 32, groupWritingApp );

  QLabel* labelProDVDKey = new QLabel( i18n("Cdrecord-ProDVD key:"), groupWritingApp );
  m_editCdrecordProDVDKey = new KLineEdit( groupWritingApp );
  QHBoxLayout* proDvdKeyLayout = new QHBoxLayout;
  proDvdKeyLayout->setSpacing( KDialog::spacingHint() );
  proDvdKeyLayout->addWidget( labelProDVDKey );
  proDvdKeyLayout->addWidget( m_editCdrecordProDVDKey );

  //
  // not used yet
  // ----------------------
  labelProDVDKey->hide();
  m_editCdrecordProDVDKey->hide();
  // ----------------------
  //

  bufferLayout->addMultiCellWidget( m_checkOverburn, 0, 0, 0, 3 );
  bufferLayout->addMultiCellWidget( m_checkManualWritingBufferSize, 1, 1, 0, 3 );
  bufferLayout->addWidget( new QLabel( "Cdrecord:", groupWritingApp ), 2, 1 );
  bufferLayout->addWidget( new QLabel( "Cdrdao:", groupWritingApp ), 3, 1 );
  bufferLayout->addWidget( m_editWritingBufferSizeCdrecord, 2, 2 );
  bufferLayout->addWidget( m_editWritingBufferSizeCdrdao, 3, 2 );
  bufferLayout->addWidget( new QLabel( i18n("MB"), groupWritingApp ), 2, 3 );
  bufferLayout->addWidget( new QLabel( i18n("blocks"), groupWritingApp ), 3, 3 );
  bufferLayout->addMultiCellWidget( m_checkAllowWritingAppSelection, 4, 4, 0, 3 );
  bufferLayout->addMultiCellLayout( proDvdKeyLayout, 5, 5, 0, 3 );
  bufferLayout->addMultiCell( new QSpacerItem( 30, 10, QSizePolicy::Fixed, QSizePolicy::Minimum ), 1, 2, 0, 0 );
  bufferLayout->setColStretch( 3, 1 );

  QGroupBox* groupMisc = new QGroupBox( 1, Qt::Vertical, i18n("Miscellaneous"), advancedTab );
  m_checkEject = new QCheckBox( i18n("Do not &eject CD after write process"), groupMisc );

  groupAdvancedLayout->addWidget( groupWritingApp, 0, 0 );
  groupAdvancedLayout->addWidget( groupMisc, 1, 0 );
  groupAdvancedLayout->setRowStretch( 2, 1 );


  connect( m_checkManualWritingBufferSize, SIGNAL(toggled(bool)),
	   m_editWritingBufferSizeCdrecord, SLOT(setEnabled(bool)) );
  connect( m_checkManualWritingBufferSize, SIGNAL(toggled(bool)),
	   m_editWritingBufferSizeCdrdao, SLOT(setEnabled(bool)) );
  connect( m_checkManualWritingBufferSize, SIGNAL(toggled(bool)),
	   this, SLOT(slotSetDefaultBufferSizes(bool)) );


  m_editWritingBufferSizeCdrecord->setDisabled( true );
  m_editWritingBufferSizeCdrdao->setDisabled( true );
  // -----------------------------------------------------------------------



  // put all in the main tabbed
  // -----------------------------------------------------------------------
  mainTabbed->addTab( projectTab, i18n("&Projects") );
  mainTabbed->addTab( advancedTab, i18n("&Advanced") );

  QToolTip::add( m_checkUseID3Tag, i18n("Rename audio files based on meta information") );
  QToolTip::add( m_checkDropDoubles, i18n("Do not ask to rename already existing files") );
  QToolTip::add( m_checkListHiddenFiles, i18n("Add hidden files in subdirectories") );
  QToolTip::add( m_checkListSystemFiles, i18n("Add system files in subdirectories") );
  QToolTip::add( m_checkAllowWritingAppSelection, i18n("Allow to choose betweeen cdrecord and cdrdao") );
  QToolTip::add( m_checkUsePbc, i18n("Playback control, PBC, is available for Video CD 2.0 and Super Video CD 1.0 disc formats.") );
  QToolTip::add( m_checkUseNumKey, i18n("Use numeric keys to navigate chapters by default (In addition to 'Previous' and 'Next')") );
  QToolTip::add( m_labelWaitTime, i18n("Time to wait after each sequence/segment by default.") );
  QToolTip::add( m_labelPlayTime, i18n("Play each sequence/segment by default.") );
  
  QWhatsThis::add( m_checkUseID3Tag, i18n("<p>If this option is checked K3b will rename audio files "
					  "that contain meta information (for example id3 tags in mp3 "
					  "files) to the following format:"
					  "<p><em>Artist - Title.extension</em>") );
  QWhatsThis::add( m_checkDropDoubles, i18n("<p>If this option is checked K3b will not ask how to "
					    "handle a file that already exists in the project "
					    "but just ignore it."
					    "<p>The default is off which means that the user is "
					    "asked to rename or ignore the file." ) );
  QWhatsThis::add( m_checkListHiddenFiles, i18n("<p>If this option is checked, hidden files "
						"in directories added to a data project will "
						"also be added.</p>" ) );
  QWhatsThis::add( m_checkListSystemFiles, i18n("<p>If this option is checked, system files "
						"(fifos, devices, sockets) "
						"in directories added to a data project will "
						"also be added.</p>" ) );
  QWhatsThis::add( m_checkAllowWritingAppSelection, i18n("<p>If this option is checked K3b gives "
							 "the possiblity to choose between cdrecord "
							 "and cdrdao when writing a cd."
							 "<p>This may be useful if one of the programs "
							 "does not support the used writer."
							 "<p><b>Be aware that K3b does not support both "
							 "programs in all project types.</b>") );
  QWhatsThis::add( m_checkUsePbc, i18n( "<p>Playback control, PBC, is available for Video CD 2.0 and Super Video CD 1.0 disc formats."
                             "<p>PBC allows control of the playback of play items and the possibility of interaction with the user through the remote control or some other input device available." ) );
}


void K3bBurningOptionTab::readSettings()
{
  KConfig* c = kapp->config();

  c->setGroup( "Video project settings" );
  m_checkUsePbc->setChecked( c->readBoolEntry("Use Playback Control", false) );
  m_spinWaitTime->setValue( c->readNumEntry( "Time to wait after each Sequence/Segment", 2 ) );
  m_spinPlayTime->setValue( c->readNumEntry( "Play each Sequence/Segment", 1 ) );
  m_checkUseNumKey->setChecked( c->readBoolEntry("Use numeric keys to navigate chapters", false) );
  
  c->setGroup( "Data project settings" );
  m_checkUseID3Tag->setChecked( c->readBoolEntry("Use ID3 Tag for mp3 renaming", false) );
  m_checkDropDoubles->setChecked( c->readBoolEntry("Drop doubles", false) );
  m_checkListHiddenFiles->setChecked( c->readBoolEntry("List hidden files", false ) );
  m_checkListSystemFiles->setChecked( c->readBoolEntry("List system files", false ) );

  c->setGroup( "Audio project settings" );
  m_editDefaultPregap->setValue( c->readNumEntry( "default pregap", 150 ) );
  m_bPregapSeconds = false;
  m_comboPregapFormat->setCurrentItem( 1 );

  c->setGroup( "General Options" );
  m_checkEject->setChecked( c->readBoolEntry( "No cd eject", false ) );
  m_checkOverburn->setChecked( c->readBoolEntry( "Allow overburning", false ) );
  bool manualBufferSize = c->readBoolEntry( "Manual buffer size", false );
  m_checkManualWritingBufferSize->setChecked( manualBufferSize );
  if( manualBufferSize ) {
    m_editWritingBufferSizeCdrecord->setValue( c->readNumEntry( "Cdrecord buffer", 4 ) );
    m_editWritingBufferSizeCdrdao->setValue( c->readNumEntry( "Cdrdao buffer", 32 ) );
  }
  m_checkAllowWritingAppSelection->setChecked( c->readBoolEntry( "Manual writing app selection", false ) );

  int defaultCdSize = c->readNumEntry( "Default cd size", 74 );
  switch( defaultCdSize ) {
  case 74:
    m_radio74Minutes->setChecked( true );
    break;
  case 80:
    m_radio80Minutes->setChecked( true );
    break;
  case 100:
    m_radio100Minutes->setChecked( true );
    break;
  default:
    m_radioCustomCdSize->setChecked( true );
    m_editCustomCdSize->setText( QString::number(defaultCdSize) );
    break;
  }

  m_editCdrecordProDVDKey->setText( c->readEntry( "cdrecord-prodvd_key" ) );
}


void K3bBurningOptionTab::saveSettings()
{
  KConfig* c = kapp->config();

  c->setGroup( "Video project settings" );
  c->writeEntry( "Use Playback Control", m_checkUsePbc->isChecked() );
  c->writeEntry( "Time to wait after each Sequence/Segment", m_spinWaitTime->value() );
  c->writeEntry( "Play each Sequence/Segment", m_spinPlayTime->value() );
  c->writeEntry( "Use numeric keys to navigate chapters", m_checkUseNumKey->isChecked() );
  
  c->setGroup( "Data project settings" );
  c->writeEntry( "Use ID3 Tag for mp3 renaming", m_checkUseID3Tag->isChecked() );
  c->writeEntry( "Drop doubles", m_checkDropDoubles->isChecked() );
  c->writeEntry( "List hidden files", m_checkListHiddenFiles->isChecked() );
  c->writeEntry( "List system files", m_checkListSystemFiles->isChecked() );

  k3bMain()->setUseID3TagForMp3Renaming( m_checkUseID3Tag->isChecked() );

  c->setGroup( "Audio project settings" );
  c->writeEntry( "default pregap", m_bPregapSeconds ? m_editDefaultPregap->value() * 75 : m_editDefaultPregap->value() );

  c->setGroup( "General Options" );
  c->writeEntry( "No cd eject", m_checkEject->isChecked() );
  c->writeEntry( "Allow overburning", m_checkOverburn->isChecked() );
  c->writeEntry( "Manual buffer size", m_checkManualWritingBufferSize->isChecked() );
  c->writeEntry( "Cdrecord buffer", m_editWritingBufferSizeCdrecord->value() );
  c->writeEntry( "Cdrdao buffer", m_editWritingBufferSizeCdrdao->value() );
  c->writeEntry( "Manual writing app selection", m_checkAllowWritingAppSelection->isChecked() );

  if( m_radio74Minutes->isChecked() )
    c->writeEntry( "Default cd size", 74 );
  else if( m_radio80Minutes->isChecked() )
    c->writeEntry( "Default cd size", 80 );
  else if( m_radio100Minutes->isChecked() )
    c->writeEntry( "Default cd size", 100 );
  if( m_radioCustomCdSize->isChecked() )
    c->writeEntry( "Default cd size", m_editCustomCdSize->text().toInt() );

  c->writeEntry( "cdrecord-prodvd_key", m_editCdrecordProDVDKey->text() );
}


void K3bBurningOptionTab::slotChangePregapFormat( const QString& format )
{
  if( format == i18n( "Seconds" ) ) {
    if( !m_bPregapSeconds ) {
      m_bPregapSeconds = true;
      m_editDefaultPregap->setValue( m_editDefaultPregap->value() / 75 );
    }
  }
  else {
    if( m_bPregapSeconds ) {
      m_bPregapSeconds = false;
      m_editDefaultPregap->setValue( m_editDefaultPregap->value() * 75 );
    }
  }
}


void K3bBurningOptionTab::slotSetDefaultBufferSizes( bool b )
{
  if( !b ) {
    m_editWritingBufferSizeCdrecord->setValue( 4 );
    m_editWritingBufferSizeCdrdao->setValue( 32 );
  }
}


#include "k3bburningoptiontab.moc"
