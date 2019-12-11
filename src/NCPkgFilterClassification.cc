/****************************************************************************
|
| Copyright (c) [2002-2011] Novell, Inc.
| All Rights Reserved.
|
| This program is free software; you can redistribute it and/or
| modify it under the terms of version 2 of the GNU General Public License as
| published by the Free Software Foundation.
|
| This program is distributed in the hope that it will be useful,
| but WITHOUT ANY WARRANTY; without even the implied warranty of
| MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.   See the
| GNU General Public License for more details.
|
| You should have received a copy of the GNU General Public License
| along with this program; if not, contact Novell, Inc.
|
| To contact Novell about this file by physical or electronic mail,
| you may find current contact information at www.novell.com
|
|***************************************************************************/


/*---------------------------------------------------------------------\
|                                                                      |
|                      __   __    ____ _____ ____                      |
|                      \ \ / /_ _/ ___|_   _|___ \                     |
|                       \ V / _` \___ \ | |   __) |                    |
|                        | | (_| |___) || |  / __/                     |
|                        |_|\__,_|____/ |_| |_____|                    |
|                                                                      |
|                               core system                            |
|                                                        (C) SuSE GmbH |
\----------------------------------------------------------------------/

   File:       NCPkgFilterRepo.cc

   Author:     Gabriele Mohr <gs@suse.com>

/-*/
#define YUILogComponent "ncurses-pkg"
#include <YUILog.h>

#include "NCPkgFilterClassification.h"

#include "YDialog.h"
#include "NCLayoutBox.h"
#include "NCSpacing.h"
#include "NCPackageSelector.h"

#include "NCZypp.h"

using std::endl;

/*
  Textdomain "ncurses-pkg"
*/


///////////////////////////////////////////////////////////////////
//
//
//	METHOD NAME : NCPkgFilterClassification::NCPkgFilterClassification
//	METHOD TYPE : Constructor
//
//	DESCRIPTION :
//

NCPkgFilterClassification::NCPkgFilterClassification( YWidget *parent, NCPackageSelector *pkg )
    :NCSelectionBox( parent, "" )
    ,packager(pkg)
{
    // fill seclection box
    suggested = new YItem( _("Suggested Packages") );
    addItem( suggested );

    recommended = new YItem( _("Recommended Packages") );
    addItem( recommended );

    orphaned = new YItem( _("Orphaned Packages") );
    addItem( orphaned );

    unneeded = new YItem( _("Unneeded Packages" ) );
    addItem( unneeded );

    multiversion = new YItem( _("Multiversion Packages" ) );
    addItem( multiversion );

    all = new YItem( _("All Packages" ) );
    addItem( all );

    showPackages();
    showDescription();
}

YItem * NCPkgFilterClassification::getCurrentGroup()
{
    int index = getCurrentItem();

    return itemAt( index );

}

bool NCPkgFilterClassification::showPackages( )
{
    NCPkgTable * packageList = packager->PackageList();

    YItem * group = getCurrentGroup();

    if ( !group )
        return false;

    if ( !packageList )
    {
	yuiError() << "No valid NCPkgTable widget" << endl;
    	return false;
    }

    // clear the package table
    packageList->itemsCleared ();

    for ( ZyppPoolIterator it = zyppPkgBegin();
          it != zyppPkgEnd();
          ++it )
    {
        ZyppSel selectable = *it;
        bool match = false;

        // If there is an installed obj, check this first. The bits are set for the installed
        // obj only and the installed obj is not contained in the pick list if there in an
        // identical candidate available from a repo.
        if ( selectable->installedObj() )
        {
           match = check( selectable, tryCastToZyppPkg( selectable->installedObj() ), group );
        }
        // otherwise display the candidate object (the "best" version)
        else if ( selectable->hasCandidateObj() )
        {
           match = check( selectable, tryCastToZyppPkg( selectable->candidateObj() ), group );
        }

        // And then check the pick list which contain all availables and all objects for multi
        // version packages and the installed obj if there isn't same version in a repo.
        if ( !match )
        {
          zypp::ui::Selectable::picklist_iterator it = selectable->picklistBegin();
          while ( it != selectable->picklistEnd() && !match )
          {
            check( selectable, tryCastToZyppPkg( *it ), group );
            ++it;
          }
        }
    }

    // show the package list
    packageList->setCurrentItem( 0 );
    packageList->drawList();
    packageList->showInformation();

    yuiMilestone() << "Filling package list \"" << group->label() <<  "\"" << endl;

    return true;
}

bool NCPkgFilterClassification::check( ZyppSel selectable, ZyppPkg pkg, YItem * group )
{
    NCPkgTable * packageList = packager->PackageList();

    // log solver bits, e.g. I__s_ou(668)libcogl11-1.12.0-1.2.i586(@System)
    yuiDebug() << zypp::PoolItem(pkg) << endl;

    if ( !packageList || !selectable || !pkg )
    {
	yuiError() << "No valid data" << endl;
    	return false;
    }

    if ( group == recommended &&
         zypp::PoolItem(pkg).status().isRecommended() )
    {
       	packageList->createListEntry( pkg, selectable );
        return true;
    }
    if ( group == suggested &&
         zypp::PoolItem(pkg).status().isSuggested() )
    {
        packageList->createListEntry( pkg, selectable );
        return true;
    }
    if ( group == orphaned &&
         zypp::PoolItem(pkg).status().isOrphaned() )
    {
        packageList->createListEntry( pkg, selectable );
        return true;
    }
    if ( group == unneeded &&
         zypp::PoolItem(pkg).status().isUnneeded() )
    {
        packageList->createListEntry( pkg, selectable );
        return true;
    }
    if ( group == multiversion &&
         selectable->multiversionInstall() )
    {
        packageList->createListEntry( pkg, selectable );
        return true;
    }
    if ( group == all )
    {
        packageList->createListEntry( pkg, selectable );
        return true;
    }

    return false;
}


void NCPkgFilterClassification::showDescription( )
{
    std::string description;

    YItem * group = getCurrentGroup();

    if ( group == recommended )
    {
        description = _("This is a list of useful packages. They will be additionally installed if recommended by a newly installed package.");
    }
    else if ( group == suggested )
    {
        description = _("It's suggested to install these packages because they fit to already installed packages. The decision to install it is by the user.");
    }
    else if ( group == orphaned )
    {
        description = _("The solver has detected that these packages are without a repository, i.e. updates aren't possible.");
    }
    else if ( group == unneeded )
    {
        description = _("These packages might be unneeded because former dependencies don't apply any longer.");
    }
    else if ( group == multiversion )
    {
        description = _("These packages might be installed in several versions in parallel.");
    }
    else if ( group == all )
    {
        description = _("All packages known by the package manager, no filtering applied.");
    }
    packager->FilterDescription()->setText ( description );
}

///////////////////////////////////////////////////////////////////
//
//
//	METHOD NAME : NCPkgFilterRepo::wHandleInput
//	METHOD TYPE : NCursesEvent
//
//	DESCRIPTION : show packages for selected group
//

NCursesEvent NCPkgFilterClassification::wHandleInput( wint_t ch )
{
    NCursesEvent ret = NCursesEvent::none;
    handleInput( ch );

    switch ( ch )
    {
	case KEY_UP:
	case KEY_DOWN:
	case KEY_NPAGE:
	case KEY_PPAGE:
	case KEY_END:
	case KEY_HOME: {
	    ret = NCursesEvent::handled;
            showPackages();
            showDescription();
	    break;
	}

	default:
	   ret = NCSelectionBox::wHandleInput( ch ) ;
     }

    return ret;
}

