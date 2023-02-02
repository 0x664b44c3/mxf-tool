#ifndef MXFMETA_H
#define MXFMETA_H
#include <QtGlobal>
#include <QDateTime>
#include <QDomDocument>
#include <QSize>
#include <QVector>
namespace MXF {

struct timeCode {
	timeCode(int h=0, int m=0, int s=0, int f=0) :
	    hours(h), minutes(m), seconds(s), frames(f) {}
	QString toString() const
	{
		return QString().sprintf("%d:%02d:%02d:%02d",
		                         hours,
		                         minutes,
		                         seconds,
		                         frames);
	}
	int hours;
	int minutes;
	int seconds;
	int frames;
};

struct MediaIndex {
	size_t startByteOffset;
	size_t dataSize;
};

struct EditUnit {
	float numerator;
	float denominator;
};

struct ClipConnection
{
	bool isSet() {
		return !clipName.isNull();
	}
	QString clipName;
	QString globalClipId;
	QString p2SerialNo;
};

struct ClipRelation
{
	int offsetInShot;
	QString globalShotId;
	ClipConnection connectionTop;
	ClipConnection connectionPrevious;
	ClipConnection connectionNext;
};

class VideoInfo
{
public:
	VideoInfo(const QDomElement &node = QDomElement());
	bool validAudioFlag() const;
	QString videoFormat() const;
	QString frameRate() const;
	QString codec() const;
	timeCode startTimecode() const;
	bool isNull() const;
	MediaIndex VideoIndex() const;

	QString aspectRatio() const;

private:
	QString mVideoFormat;
	QString mFrameRate;
	QString mCodec;
	QString mAspectRatio;
	timeCode mStartTimecode;
	QString mStartBinaryGroup;
	MediaIndex mVideoIndex;
	bool mValidAudio;
	bool mIsNull;
};

struct AudioInfo {
public:
	AudioInfo(const QDomElement & node = QDomElement());
	QString audioFormat() const;
	int samplingRate() const;
	int bitsPerSample() const;
	MediaIndex audioIndex() const;

private:
	QString mAudioFormat;
	int mSamplingRate;
	int mBitsPerSample;
	MediaIndex mAudioIndex;
};

struct DeviceInfo
{
	QString manufacturer;
	QString serialNo;
	QString modelName;
};
struct ThumbNailInfo
{
	int frameOffset;
	QString format;
	QSize size;
};

class ClipMetaData
{
public:
	ClipMetaData(const QDomElement & node = QDomElement());

	QString userClipName() const;
	QString dataSource() const;
	QDateTime creationDate() const;
	QDateTime lastUpdate() const;
	DeviceInfo device() const;
	QDateTime shootStart() const;
	QDateTime shootEnd() const;
	ThumbNailInfo thumbnail() const;

private:
	QString mUserClipName;
	QString mDataSource;
	QDateTime mCreationDate;
	QDateTime mLastUpdate;
	DeviceInfo mDevice;
	QDateTime mShootStart;
	QDateTime mShootEnd;

	ThumbNailInfo mThumbnail;
};

class ClipInfo {
public:
	ClipInfo(const QByteArray & xmlData = QByteArray());
	QString clipName() const;
	QString globalClipID() const;
	int duration() const;
	EditUnit editUnit() const;
	ClipRelation relation() const;
	VideoInfo videoEssence() const;
	QVector<AudioInfo> audioEssences() const;

	ClipMetaData metaData() const;

	bool isNull() const;

private:
	QString mClipName;
	QString mGlobalClipID;
	EditUnit mEditUnit;
	int mDuration;
	ClipRelation mRelation;
	VideoInfo mVideoEssence;
	QVector<AudioInfo> mAudioEssences;
	ClipMetaData mMetaData;
	bool mIsNull;
};

}


#endif // MXFMETA_H
