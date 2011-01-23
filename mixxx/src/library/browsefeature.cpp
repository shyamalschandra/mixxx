// browsefeature.cpp
// Created 9/8/2009 by RJ Ryan (rryan@mit.edu)

#include <QStringList>
#include <QTreeView>
#include <QDirModel>
#include <QStringList>
#include <QFileInfo>

#include "trackinfoobject.h"
#include "library/treeitem.h"
#include "library/browsefeature.h"
#include "library/browsefilter.h"
#include "library/libraryview.h"
#include "library/trackcollection.h"
#include "library/dao/trackdao.h"
#include "widget/wwidget.h"
#include "widget/wskincolor.h"
#include "widget/wlibrary.h"
#include "widget/wlibrarysidebar.h"
#include "widget/wlibrarytableview.h"
#include "widget/wbrowsetableview.h"

BrowseFeature::BrowseFeature(QObject* parent, ConfigObject<ConfigValue>* pConfig, TrackCollection* pTrackCollection)
        : LibraryFeature(parent),
          m_pConfig(pConfig),
          m_browseModel(this),
          m_proxyModel(&m_browseModel),
          m_pTrackCollection(pTrackCollection) {
     
    m_proxyModel.setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel.setSortCaseSensitivity(Qt::CaseInsensitive);
               
    //The invisible root item of the child model
    TreeItem* rootItem = new TreeItem();
    
    /*
     * Just a word about how the TreeItem objects are used for the BrowseFeature:
     * The constructor has 4 arguments:
     * 1. argument represents the folder name shown in the sidebar later on
     * 2. argument represents the folder path which MUST end with '/'
     * 3. argument is the library feature itself
     * 4. the parent TreeItem object
     *
     * Except the invisible root item, you must always state all 4 arguments.
     *
     * Once the TreeItem objects are inserted to models, the models take care of their 
     * deletion.
     */
      
    //Add a shortcut to the Music folder which Mixxx uses
    QString mixxx_music_dir = m_pConfig->getValueString(ConfigKey("[Playlist]","Directory"));
    TreeItem* mixxx_music_dir_item = new TreeItem("My Music", mixxx_music_dir +"/" ,this , rootItem);
    rootItem->appendChild(mixxx_music_dir_item);
    
#if defined(__WINDOWS__)
    QFileInfoList drives = QDir::drives();
    //show drive letters
    foreach(QFileInfo drive, drives){
        TreeItem* driveLetter = new TreeItem(
                        drive.canonicalPath(), // displays C:
                        drive.filePath(), //Displays C:/
                        this , 
                        rootItem);
        rootItem->appendChild(driveLetter);
    }
#elif defined(__APPLE__)
    //Apple hides the base Linux file structure
    //But all devices are mounted at /Volumes
    TreeItem* devices = new TreeItem("Devices", "/Volumes/", this , rootItem);
    rootItem->appendChild(devices);
#else
    //show root directory on UNIX-based operating systems
    TreeItem* root_folder_item = new TreeItem(QDir::rootPath(), QDir::rootPath(),this , rootItem);
    rootItem->appendChild(root_folder_item);
#endif
    
    m_childModel.setRootItem(rootItem);
}

BrowseFeature::~BrowseFeature() {
    
}

QVariant BrowseFeature::title() {
    return QVariant(tr("Browse"));
}

QIcon BrowseFeature::getIcon() {
    return QIcon(":/images/library/ic_library_browse.png");
}

TreeItemModel* BrowseFeature::getChildModel() {
    return &m_childModel;
}

bool BrowseFeature::dropAccept(QUrl url) {
    return false;
}

bool BrowseFeature::dropAcceptChild(const QModelIndex& index, QUrl url) {
    return false;
}

bool BrowseFeature::dragMoveAccept(QUrl url) {
    return false;
}

bool BrowseFeature::dragMoveAcceptChild(const QModelIndex& index, QUrl url) {
    return false;
}

void BrowseFeature::activate() {
    emit(switchToView("BROWSE"));
    emit(restoreSearch(m_currentSearch));
}

void BrowseFeature::activateChild(const QModelIndex& index) {
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    qDebug() << "BrowseFeature::activateChild " << item->data() << " " << item->dataPath();
    
    //populate childs
    QDir dir(item->dataPath().toString());
    QFileInfoList all = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);

    // loop through all the item and construct the childs
    QList<TreeItem*> folders;
    foreach(QFileInfo one, all){
        //Skip folders that end with .app on OS X
        #if defined(__APPLE__)
        if(one.isDir() && one.fileName().endsWith(".app")) continue;
        #endif
        // We here create new items for the sidebar models
        // Once the items are added to the TreeItemModel, 
        // the models takes ownership of them and ensures their deletion
        TreeItem* folder = new TreeItem(one.fileName(), item->dataPath().toString().append(one.fileName() +"/"), this, item);
        folders << folder;  
    }
    //Before we populate the subtree, we need to delete old subtrees
    m_childModel.removeRows(0, item->childCount(), index);
    
    //we need to check here if subfolders are found
    //On Ubuntu 10.04, otherwise, this will draw an icon although the folder has no subfolders
    if(!folders.isEmpty())
        m_childModel.insertRows(folders, 0, folders.size() , index);
    /*
    QModelIndex startIndex = m_browseModel.index(item->dataPath().toString());
    QModelIndex proxyIndex = m_proxyModel.mapFromSource(startIndex);
    emit(setRootIndex(proxyIndex));
    emit(switchToView("BROWSE"));
    */
    m_browseModel.setPath(item->dataPath().toString());
    //emit(switchToView("BROWSE"));
    //emit(showTrackModel(&m_browseModel));
    emit(showTrackModel(&m_proxyModel));
    

}

void BrowseFeature::onRightClick(const QPoint& globalPos) {
}

void BrowseFeature::onRightClickChild(const QPoint& globalPos, QModelIndex index) {
}