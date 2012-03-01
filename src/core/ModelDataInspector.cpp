
#include <QAbstractItemModel>
#include <QFileInfo>
#include <QRegExp>
#include <QSettings>
#include <QStringList>

#include "MetaDataModel2.h"
#include "ModelDataInspector.h"

#include "globals.h"

#include <QDebug>

typedef MetaDataModel2 MetaDataModel;

ModelDataInspector::ModelDataInspector(QObject * parent=0)
	: QObject(parent), _model(0) {

}

void ModelDataInspector::setModel(QAbstractItemModel * model) {

	if (_model)  {
		disconnect(_model, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
			this, SLOT(modelDataChanged(const QModelIndex &, const QModelIndex &)));

		disconnect(_model, SIGNAL(rowsInserted(const QModelIndex &, int, int)),
			this, SLOT(modelRowsInserted(const QModelIndex &, int, int)));
	}

	connect(model, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
		SLOT(modelDataChanged(const QModelIndex &, const QModelIndex &)));

	connect(model, SIGNAL(rowsInserted(const QModelIndex &, int, int)),
		SLOT(modelRowsInserted(const QModelIndex &, int, int)));

	_model = model;
}

void ModelDataInspector::modelDataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight) {
	checkData(topLeft.row(), bottomRight.row(), topLeft.parent());
}

void ModelDataInspector::modelRowsInserted(const QModelIndex & parent, int start, int end) {
	checkData(start, end, parent);
}

void ModelDataInspector::checkData(int firstRow, int lastRow, const QModelIndex & parent) {
	if (parent.isValid() || !model())
		return;
		
	disconnect(_model, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
		this, SLOT(modelDataChanged(const QModelIndex &, const QModelIndex &)));

	QSettings s;

	bool lowerCaseExtension = s.value("LowerCaseExtension").toBool();
	
	bool safeFileNames = s.value("safeFileNames").toBool();
	bool discNumberFromAlbum = s.value("ParseDiscNumber").toBool();
	bool trimWhiteSpace = s.value("TrimWhiteSpace").toBool();
	bool capitalizeRoman = s.value("CapitalizeRomanNumerals").toBool();
	
	for (int i = firstRow; i <= lastRow; i++) {

		// Trim white-space from all string fields
		if (trimWhiteSpace) {
			QList<int> fields = QList<int>() << MetaData::TitleField
				<< MetaData::ArtistField << MetaData::AlbumField
				<< MetaData::GenreField << MetaData::OriginalArtistField
				<< MetaData::ComposerField << MetaData::UrlField
				<< MetaData::EncoderField;

			foreach (int field, fields) {
				const QModelIndex idx = _model->index(i, field);
				
				QString value = idx.data().toString();
				value = value.replace(QRegExp("\\s+"), " ").trimmed();

				_model->setData(idx, value);
			}
			
		}
		
		// Convert filetype extension to user-defined casing.
		if (lowerCaseExtension) {
			const QModelIndex idx = _model->index(i, MetaData::PathField);
			QString fileName = idx.data(Qt::EditRole).toString();
			QString suffix = QFileInfo(fileName).suffix();

			fileName.replace(fileName.length()-suffix.length(), suffix.length(), suffix.toLower());

			_model->setData(idx, fileName);
		}

		// Remove all special characters from filenames.
		// Only letters, numbers and '-' and '_' and '.' are allowed.
		if (safeFileNames) {
			const QModelIndex idx = _model->index(i, MetaData::PathField);
			QFileInfo info(idx.data(Qt::EditRole).toString());
			QString fileName = info.fileName();

			fileName = fileName.replace(QRegExp("[^\\w\\-\\. ]"), "");
			fileName = QString("%1/%2").arg(info.absolutePath(), fileName);

			_model->setData(idx, fileName);
		}

		// Try to parse disc number from album field.
		// Appending disc number into album name has been a common habit at least
		// in the past.
		// Searches for strings "(Disc %d)" and "(CD %d)" at the end of the album name.
		if (discNumberFromAlbum) {
			const QModelIndex idx = _model->index(i, MetaData::AlbumField);
			QString album = idx.data().toString();

			QRegExp rx("\\s*\\((?:cd|disc)\\s*(\\d+)\\)", Qt::CaseInsensitive);
			
			if (rx.indexIn(album) > -1) {
				int discNumber = rx.cap(1).toInt();

				qDebug() << "set disc" << discNumber;

				_model->setData(idx, album.remove(rx));
				_model->setData(idx.sibling(idx.row(), MetaData::DiscNumberField), discNumber);
			}
		}
	}

	/*
	for (int i = firstRow; i <= lastRow; i++) {
		const QModelIndex idx = _model->index(i, 0, parent);
		QMap<int, QString> fields;

		QAbstractItemModel * mdm = _model;
		
		for (int j = 0; j < mdm->columnCount(); j++)
			fields.insert(j, mdm->columnName(j));

		fields.remove( modelColumn("Disc") );
		fields.remove( modelColumn("Number") );
		fields.remove( modelColumn("MaxNumber") );
		fields.remove( modelColumn("Path") );
		fields.remove( modelColumn("Year") );
		fields.remove( modelColumn("Pictures") );

		if (Coquillo::safeFileNames) {
			const QModelIndex idx = _model->index(i, modelColumn("Path"));
			const QRegExp chars("[\"*:<>\?\\|]");

			QString path = idx.data(Qt::EditRole).toString();
			const QString dir = path.section('/', 0, -2);
			const QString name = path.section('/', -1).remove(chars);

			path = QString("%1/%2").arg(dir, name);

			_model->setData( _model->index(i, modelColumn("Path")), path );
		}

		if (Coquillo::fileExtensionCase) {
			const QModelIndex idx = _model->index(i, modelColumn("Path"));
			QString path = idx.data(Qt::EditRole).toString();
			const QString suffix = path.section('.', -1);

			path.chop(suffix.length());

			if (Coquillo::fileExtensionCase == 1)
				path.append(suffix.toLower());
			else
				path.append(suffix.toUpper());

			_model->setData(idx, path);
		}

		if (Coquillo::removeDiscFromAlbumName) {
			const QModelIndex idx = _model->index(i, modelColumn("Album"));
			QRegExp r("\\s*[\\[\\(](?:disc|cd)\\s*(\\d+)[\\]\\)]\\s*$");

			r.setCaseSensitivity(Qt::CaseInsensitive);

			QString albumName = idx.data(Qt::EditRole).toString();

			if (r.indexIn(albumName) != -1) {
				int disc = r.cap(1).toInt();
				albumName.remove(r);

				_model->setData(idx, albumName);
				_model->setData(idx.sibling(idx.row(), modelColumn("Disc")), disc);
			}



		}

		foreach (int col, fields.keys()) {

			const QModelIndex idx = _model->index(i, col);
			QString value = idx.data(Qt::EditRole).toString();

			if (Coquillo::trimWhiteSpace)
				value = value.trimmed().replace(QRegExp("\\s+"), QString(' '));

			_model->setData(idx, value);

		}

	}
	*/

	connect(_model, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
		this, SLOT(modelDataChanged(const QModelIndex &, const QModelIndex &)));
}
