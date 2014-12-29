
#include <QVariantMap>
#include <taglib/tstring.h>
#include "abstracttag.h"

#define T2QString(str) QString::fromUtf8((str).toCString(true))
#define Q2TString(str) TagLib::String((str).toUtf8().data(), TagLib::String::UTF8)

namespace Coquillo {
    namespace MetaData {
        namespace Container {
            AbstractTag::AbstractTag(TagLib::Tag * tag)
            : _tag(tag) {

            }

            QVariantMap AbstractTag::readCommon() const {
                QVariantMap data;
                data.insert("album", T2QString(_tag->album()));
                data.insert("artist", T2QString(_tag->artist()));
                data.insert("comment", T2QString(_tag->comment()));
                data.insert("genre", T2QString(_tag->genre()));
                data.insert("number", _tag->track());
                data.insert("title", T2QString(_tag->title()));
                data.insert("year", _tag->year());
                return data;
            }

            void AbstractTag::writeCommon(const QVariantMap & data) {
                _tag->setAlbum(Q2TString(data.value("album").toString()));
                _tag->setArtist(Q2TString(data.value("artist").toString()));
                _tag->setComment(Q2TString(data.value("comment").toString()));
                _tag->setTrack(data.value("number").toUInt());
                _tag->setTitle(Q2TString(data.value("title").toString()));
                _tag->setYear(data.value("year").toUInt());
            }
        }
    }
}
