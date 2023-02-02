#include "mxfmeta.h"
#include <QDomDocument>
#include <QDomElement>
#include <QDomNode>
#include <QStringList>
#include <QDomDocument>
#include <QDebug>
#define QSL(a) QStringLiteral(a)

namespace MXF
{


static timeCode parseTimeCode(QString tc)
{
	timeCode out;
	QStringList elements = tc.split(':');
	if (elements.size() != 4)
		return timeCode();
	bool ok;
	out.hours = elements[0].toInt(&ok);
	if (!ok)
		return timeCode();
	out.minutes = elements[1].toInt(&ok);
	if (!ok)
		return timeCode();
	out.seconds = elements[2].toInt(&ok);
	if (!ok)
		return timeCode();
	out.frames  = elements[3].toInt(&ok);
	if (!ok)
		return timeCode();
	return out;
}

static MediaIndex parseIndex(const QDomElement & e)
{
	MediaIndex idx;
	idx.startByteOffset =
	        e.firstChildElement(QStringLiteral("StartByteOffset")).text().toULongLong();
	idx.dataSize =
	        e.firstChildElement(QStringLiteral("DataSize")).text().toULongLong();
	return idx;
}

static ClipConnection parseConnection (const QDomElement & e)
{
	ClipConnection conn;
	if (e.isNull())
		return conn;
	conn.clipName =
	        e.firstChildElement(QStringLiteral("ClipName")).text();
	conn.globalClipId =
	        e.firstChildElement(QStringLiteral("GlobalClipID")).text();
	conn.p2SerialNo =
	        e.firstChildElement(QStringLiteral("P2SerialNo.")).text();
	return conn;
}

static ClipRelation parseRelation(const QDomElement & e)
{
	ClipRelation relation;
	relation.offsetInShot = 0;
	if (e.isNull())
		return relation;
	relation.offsetInShot =
	        e.firstChildElement(QStringLiteral("OffsetInShot")).text().toInt();
	relation.globalShotId =
	        e.firstChildElement(QStringLiteral("GlobalShotID")).text();
	QDomElement conn = e.firstChildElement(QStringLiteral("Connection"));
	relation.connectionTop =
	        parseConnection(conn.firstChildElement(QStringLiteral("Top")));
	relation.connectionPrevious =
	        parseConnection(conn.firstChildElement(QStringLiteral("Previous")));
	relation.connectionNext =
	        parseConnection(conn.firstChildElement(QStringLiteral("Next")));
}

//video entry in essence list

VideoInfo::VideoInfo(const QDomElement &node)
{
	mIsNull = true;

	if (node.hasAttribute("ValidAudioFlag"))
		mValidAudio = node.attribute("ValidAudioFlag").toLower() == "true";
	else
		mValidAudio = true;

	mVideoFormat = node.firstChildElement(QStringLiteral("VideoFormat")).text();
	mCodec       = node.firstChildElement(QStringLiteral("Codec")).text();
	mFrameRate   = node.firstChildElement(QStringLiteral("FrameRate")).text();
	mAspectRatio = node.firstChildElement(QStringLiteral("AspectRatio")).text();
	mStartTimecode = parseTimeCode(
	                     node.firstChildElement(QStringLiteral("StartTimecode")).text()
	                     );
	mStartBinaryGroup =
	        node.firstChildElement(QStringLiteral("StartBinaryGroup")).text();
	mVideoIndex = parseIndex(node.firstChildElement(QStringLiteral("VideoIndex")));

	mIsNull = false;
}

QString VideoInfo::videoFormat() const
{
    return mVideoFormat;
}

QString VideoInfo::frameRate() const
{
    return mFrameRate;
}

QString VideoInfo::codec() const
{
    return mCodec;
}

timeCode VideoInfo::startTimecode() const
{
    return mStartTimecode;
}

bool VideoInfo::isNull() const
{
	return mIsNull;
}

MediaIndex VideoInfo::VideoIndex() const
{
	return mVideoIndex;
}

QString VideoInfo::aspectRatio() const
{
	return mAspectRatio;
}


//audio entry in essence list
AudioInfo::AudioInfo(const QDomElement &node) :
    mSamplingRate(0), mBitsPerSample(0)
{
	if (node.isNull())
		return;
	mAudioFormat = node.firstChildElement(QStringLiteral("AudioFormat")).text();
	mSamplingRate= node.firstChildElement(QStringLiteral("SamplingRate")).text().toInt();
	mBitsPerSample= node.firstChildElement(QStringLiteral("FrameRate")).text().toInt();
}

QString AudioInfo::audioFormat() const
{
	return mAudioFormat;
}
int AudioInfo::samplingRate() const
{
	return mSamplingRate;
}
int AudioInfo::bitsPerSample() const
{
	return mBitsPerSample;
}
MediaIndex AudioInfo::audioIndex() const
{
	return mAudioIndex;
}

ClipInfo::ClipInfo(const QByteArray &xmlData) :
    mIsNull(true)
{
	QDomDocument doc;
	QString errorMsg;
	int errorLine = -1, errorColumn = -1;
	bool domOk = doc.setContent(xmlData, &errorMsg, &errorLine, & errorColumn);
	if (!domOk)
	{
		qCritical() << "Failed reading XML file:" <<
		               errorMsg << "in" << errorLine << ":" << errorColumn;
		return;
	}
	if (!doc.childNodes().count())
	{
		return;
	}
	QDomNode root = doc.documentElement();
	if (root.nodeName() != "P2Main")
		return;
	QDomElement clipContent = root.firstChildElement(QStringLiteral("ClipContent"));
	if (clipContent.isNull())
		return;

	mIsNull = false;
	//ok to start and parse
	mClipName =
	        clipContent.firstChildElement(QStringLiteral("ClipName")).text();
	mGlobalClipID =
	        clipContent.firstChildElement(QStringLiteral("GlobalClipID")).text();
	mDuration =
	        clipContent.firstChildElement(QStringLiteral("Duration")).text().toInt();
	QStringList units =
	        clipContent.firstChildElement(QStringLiteral("EditUnit")).text().split('/');
	mEditUnit.numerator = units.first().toFloat();
	mEditUnit.denominator = (units.count()>1)?units.last().toFloat():1;

	QDomElement relationNode = clipContent.firstChildElement(QStringLiteral("Relation"));
	mRelation = parseRelation(relationNode);

	QDomElement essenceNode = clipContent.firstChildElement(QStringLiteral("EssenceList"));
	bool hasVideo=false;
	QDomNodeList essences = essenceNode.childNodes();
	for (int i=0; i < essences.length(); ++i)
	{
		QDomNode e = essences.at(i);
		if (!e.isElement())
			continue;
		if ((e.nodeName() == "Video") && (!hasVideo))
			mVideoEssence = VideoInfo(e.toElement());
		if (e.nodeName() == "Audio")
			mAudioEssences.push_back(AudioInfo(e.toElement()));
	}
	QDomElement metaNode = clipContent.firstChildElement(QStringLiteral("ClipMetadata"));
	mMetaData = ClipMetaData(metaNode);
}

QString ClipInfo::clipName() const
{
	return mClipName;
}

QString ClipInfo::globalClipID() const
{
	return mGlobalClipID;
}

int ClipInfo::duration() const
{
	return mDuration;
}

ClipRelation ClipInfo::relation() const
{
	return mRelation;
}

VideoInfo ClipInfo::videoEssence() const
{
	return mVideoEssence;
}

QVector<AudioInfo> ClipInfo::audioEssences() const
{
	return mAudioEssences;
}

ClipMetaData ClipInfo::metaData() const
{
	return mMetaData;
}

bool ClipInfo::isNull() const
{
    return mIsNull;
}

EditUnit ClipInfo::editUnit() const
{
    return mEditUnit;
}

ClipMetaData::ClipMetaData(const QDomElement &node)
{
	if (node.isNull())
		return;
	mUserClipName =
	        node.firstChildElement(QStringLiteral("UserClipName")).text();
	mDataSource =
	        node.firstChildElement(QStringLiteral("DataSource")).text();
	QDomElement accessNode =
	        node.firstChildElement(QStringLiteral("Access"));
	if (!accessNode.isNull())
	{
		mCreationDate =
		        QDateTime::fromString(
		            accessNode.firstChildElement(QStringLiteral("CreationDate")).text()
		            , Qt::ISODate);
		mLastUpdate =
		        QDateTime::fromString(
		            accessNode.firstChildElement(QStringLiteral("LastUpdateDate")).text()
		            , Qt::ISODate);
	}
	else
	{
		mCreationDate = mLastUpdate = QDateTime::currentDateTime();
	}
	QDomElement deviceNode =
	        node.firstChildElement(QStringLiteral("Device"));
	if (!deviceNode.isNull())
	{
		mDevice.manufacturer =
		        deviceNode.firstChildElement(QStringLiteral("Manufacturerer")).text();
		mDevice.serialNo =
		        deviceNode.firstChildElement(QStringLiteral("SerialNo.")).text();
		mDevice.modelName =
		        deviceNode.firstChildElement(QStringLiteral("ModelName")).text();
	}
	QDomElement shootNode =
	        node.firstChildElement(QStringLiteral("Shoot"));
	if (!shootNode.isNull())
	{
		mShootStart = QDateTime::fromString(
		        shootNode.firstChildElement(QStringLiteral("StartDate")).text(),
		                  Qt::ISODate);
		mShootEnd   = QDateTime::fromString(
		        shootNode.firstChildElement(QStringLiteral("EndDate")).text(),
		                  Qt::ISODate);
	}
	QDomElement thumbNailNode =
	        node.firstChildElement(QStringLiteral("Thumbnail"));
	if (!thumbNailNode.isNull())
	{
		mThumbnail.frameOffset =
		        thumbNailNode.firstChildElement(QStringLiteral("FrameOffset")).text().toInt();
		mThumbnail.format =
		        thumbNailNode.firstChildElement(QStringLiteral("ThumbnailFormat")).text();
		int width =
		        thumbNailNode.firstChildElement(QStringLiteral("Width")).text().toInt();
		int height =
		        thumbNailNode.firstChildElement(QStringLiteral("Width")).text().toInt();
		mThumbnail.size = QSize(width, height);
	}


}

QString ClipMetaData::userClipName() const
{
	return mUserClipName;
}


QString ClipMetaData::dataSource() const
{
	return mDataSource;
}

QDateTime ClipMetaData::creationDate() const
{
	return mCreationDate;
}

DeviceInfo ClipMetaData::device() const
{
	return mDevice;
}

QDateTime ClipMetaData::shootStart() const
{
	return mShootStart;
}

QDateTime ClipMetaData::shootEnd() const
{
	return mShootEnd;
}

ThumbNailInfo ClipMetaData::thumbnail() const
{
	return mThumbnail;
}

QDateTime ClipMetaData::lastUpdate() const
{
	return mLastUpdate;
}


}
