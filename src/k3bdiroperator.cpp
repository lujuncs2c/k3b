/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3bdiroperator.h"

#include <k3bcore.h>

#include <kcombiview.h>
#include <kfilepreview.h>
#include <kaction.h>
#include <kbookmarkmenu.h>
#include <kstandarddirs.h>
#include <kpopupmenu.h>

#include <qdir.h>


K3bDirOperator::K3bDirOperator(const KURL& url, QWidget* parent, const char* name )
  : KDirOperator( url, parent, name )
{
  // disable the del-key since we still have a focus problem and users keep
  // deleting files when they want to remove project entries
  KAction* aDelete = actionCollection()->action("delete");
  if( aDelete )
    aDelete->setAccel( 0 );

  // add the bookmark stuff
  KBookmarkManager* bmMan = KBookmarkManager::managerForFile( locateLocal( "data", "k3b/bookmarks.xml" ), false );
  bmMan->setEditorOptions( i18n("K3b Bookmarks"), false );
  bmMan->setUpdate( true );
  bmMan->setShowNSBookmarks( false );

  m_bmPopup = new KActionMenu( i18n("Bookmarks"), "bookmark", this, "bookmarks" );
  KActionMenu* dirOpMenu = (KActionMenu*)actionCollection()->action("popupMenu");
  dirOpMenu->insert( new KActionSeparator( actionCollection() ) );
  dirOpMenu->insert( m_bmPopup );
  m_bmMenu = new KBookmarkMenu( bmMan, this, m_bmPopup->popupMenu(), actionCollection(), true );
}


K3bDirOperator::~K3bDirOperator()
{
  delete m_bmMenu; 
}


void K3bDirOperator::readConfig( KConfig* cfg, const QString& group )
{
  QString oldGroup = cfg->group();
  cfg->setGroup( group );

  KDirOperator::readConfig( cfg, group );

  //
  // There seems to be a bug in the KDELibs which makes setURL crash on
  // some systems when used with a non-existing url
  //
  QString lastUrl = cfg->readPathEntry( "last url", QDir::home().absPath() );
  if( !QFile::exists(lastUrl) )
    lastUrl = QDir::home().absPath();
  setURL( KURL::fromPathOrURL(lastUrl), true );

  cfg->setGroup( oldGroup );

  emit urlEntered( url() );
}


void K3bDirOperator::writeConfig( KConfig* cfg, const QString& group )
{
  QString oldGroup = cfg->group();
  cfg->setGroup( group );

  KDirOperator::writeConfig( cfg, group );
  cfg->writePathEntry( "last url", url().path() );

  cfg->setGroup( oldGroup );
}


void K3bDirOperator::openBookmarkURL( const QString& url )
{
  setURL( KURL::fromPathOrURL( url ), true );
}


QString K3bDirOperator::currentTitle() const
{
  return url().path(-1);
}


QString K3bDirOperator::currentURL() const
{
  return url().path(-1);
}


void K3bDirOperator::activatedMenu( const KFileItem* item, const QPoint& pos )
{
  // TODO: use our own menu and remove or add play and stuff
  return KDirOperator::activatedMenu( item, pos );
}


#include "k3bdiroperator.moc"

