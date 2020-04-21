#include "tracklistmodel.h"
#include "track.h"
#include "qganttmodel.h"

#include "live/visuallogqt.h"

namespace lv{

TrackListModel::TrackListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_roles[TrackRole] = "track";
}

TrackListModel::~TrackListModel(){
    clearTracks();
}

QVariant TrackListModel::data(const QModelIndex& index, int role) const{
    if ( role == TrackListModel::TrackRole)
        return QVariant::fromValue(m_tracks[index.row()]);
    return QVariant();
}

void TrackListModel::clearTracks(){
    beginResetModel();
    for ( Track* track : m_tracks ){
        track->deleteLater();
        track->setParent(nullptr);
    }
    m_tracks.clear();
    endResetModel();
}

Track* TrackListModel::appendTrack(){
    Track* track = new Track(this);
    track->setName("Track #" + QString::number(m_tracks.length() + 1));
    appendTrack(track);
    return track;
}

lv::Track *TrackListModel::trackAt(int index) const{
    return m_tracks.at(index);
}

int TrackListModel::totalTracks() const{
    return m_tracks.size();
}

void TrackListModel::removeTrack(Track *track){
    if ( !track )
        return;
    int index = m_tracks.indexOf(track);
    if ( index == -1 )
        return;
    removeTrack(index);
}

void TrackListModel::removeTrack(int index){
    beginRemoveRows(QModelIndex(), index, index);
    delete m_tracks.takeAt(index);
    endRemoveRows();
}

void TrackListModel::appendTrack(Track *track){
    beginInsertRows(QModelIndex(), m_tracks.length(), m_tracks.length());
    m_tracks.append(track);
    endInsertRows();
}

}// namespace
