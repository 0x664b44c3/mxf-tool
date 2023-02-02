#include "wndmain.h"
#include <QApplication>
#include <QXmlStreamReader>
#include <QFile>
#include <QDebug>
#include <QDir>
#include <QProcess>
namespace MXF {

struct VideoInfo
{
	QString Filename;
	QString VideoFormat;
	QString FrameRate;
	QString StartTimecode;
};

struct AudioInfo {
	QString AudioFormat;
};

struct Info {
	QString clipName;
	QString GlobalClipID;
	int duration;
	struct {
		float numerator;
		float denominator;
	} EditUnit;
	VideoInfo Video;
	QVector<QString> AudioChannel;
	QString ThumbnailFile;
};

}

bool parseMeta(QByteArray data, MXF::Info & mxf) {
	int audioTrack = 0;
	QXmlStreamReader xml;
	xml.addData(data);
	int depth=0;
	QStringList path;
	path.append("");
	while((!xml.atEnd())&&(!xml.error())) {
		xml.readNext();
		QString xpath;
		if (xml.error()) {
			//qDebug()<< trUtf8("XML error: %1").arg(xml.errorString());
			break;
		}
		if (xml.isStartElement()) {
			path.append(xml.name().toString());
			depth++;
			continue;
		}
		if (xml.isEndElement()) {
			path.removeLast();
			depth--;
			continue;
		}
		xpath = path.join("/");
		if (xpath == "/P2Main/ClipContent/ClipName")
			mxf.clipName = xml.text().toString();
		if (xpath == "/P2Main/ClipContent/GlobalClipId")
			mxf.GlobalClipID = xml.text().toString();
		if (xpath == "/P2Main/ClipContent/Duration")
			mxf.duration = xml.text().toString().toInt();
		if (xpath == "/P2Main/ClipContent/EditUnit")
		{
			QString eu = xml.text().toString();
			QStringList units = eu.split('/');
			mxf.EditUnit.numerator = units.first().toFloat();
			mxf.EditUnit.denominator = (units.count()>1)?units.last().toFloat():1;
		}

		//skip relation for now except media serial
		//<P2SerialNo.>ABD06C0058</P2SerialNo.>

		if (xpath == "/P2Main/ClipContent/EssenceList/Video/VideoFormat")
			mxf.Video.Filename = xml.text().toString();

		if (xpath == "/P2Main/ClipContent/EssenceList/Audio/AudioFormat")
		{
			QString audioName;
			audioName.sprintf("%02x.", audioTrack++);
			audioName += xml.text().toString();
			mxf.AudioChannel.append(audioName);
		}

		if (xpath =="/P2Main/ClipContent/ClipMetadata/Thumbnail/ThumbnailFormat")
			mxf.ThumbnailFile = xml.text().toString();

		//qDebug()<<path.join("/")<<xml.text();
	}
	if (xml.error()) {
		//QDebug()<< trUtf8("XML parser error: %1").arg(xml.errorString());
		return false;
	}else{
		mxf.Video.Filename.prepend(mxf.clipName+".");
		mxf.ThumbnailFile.prepend(mxf.clipName+".");
		for (int i=0;i<audioTrack;++i)
			mxf.AudioChannel[i].prepend(mxf.clipName);
		return true;
	}

}

QStringList convertFolderCmds(QString cardRoot, QString outputPath, int maxAudio = 1)
{
	QDir dir(cardRoot);
	QStringList cmdList;
	if (!dir.entryList().contains("CONTENTS"))
		dir.cd("..");
	cardRoot = dir.absolutePath();
	if (!dir.entryList().contains("CONTENTS"))
		return QStringList();
	QString cardId = dir.absolutePath().split("/").last();
	QString fileRoot = cardRoot + "/CONTENTS/";
	dir.setPath(cardRoot+"/CONTENTS/CLIP");
	dir.setNameFilters(QStringList()<<"*.xml"<<"*.XML");
	qDebug()<<dir.absolutePath()<<dir;
	foreach(QFileInfo file, dir.entryInfoList())
	{
		QString fileName = file.filePath();
		QFile mxf(fileName);
		if (!mxf.open(QFile::ReadOnly))
			continue;

		QByteArray xmlData = mxf.readAll();
		MXF::Info info;
		parseMeta(xmlData, info);

		//qDebug() << info.Video.Filename << info.AudioChannel;
		QStringList arguments;
		//arguments.append("ffmpeg");
		arguments.append("-i");

		arguments.append(fileRoot + "VIDEO/" + info.Video.Filename);

		//audio mapping
		int nAudioChans = (maxAudio>0)?
						 qMin(info.AudioChannel.count(),maxAudio)
						:info.AudioChannel.count();

		for (int chan = 0; chan < nAudioChans; ++chan)
		{
			arguments.append("-i");
			arguments.append(fileRoot + "AUDIO/" + info.AudioChannel[chan]);
		}

		//codec
		arguments.append("-c:v");
		arguments.append("copy");
		arguments.append("-c:a");
		arguments.append("copy");

		//mapping
		arguments.append("-map");
		arguments.append("0:v");
		for (int audioId=0; audioId < nAudioChans; ++audioId)
		{
			QString mapping;
			mapping.sprintf("1:a");//, audioId+1, audioId);
			arguments.append("-map");
			arguments.append(mapping);
		}
		QString output = outputPath + cardId+"_"+info.clipName + ".avi";
		arguments.append(output);

		//mapping
		cmdList.append(arguments.join(" "));

	}

	return cmdList;
}

int main(int argc, char *argv[])
{

	QString path = QDir::currentPath();
	QString outPath = path;
	if (argc>1)
	{
		path = argv[1];
		QDir sd(path);
		if (!sd.exists())
			return 2;
		path = sd.absolutePath();
		qDebug() << path;
	}
	if (argc>2)
		outPath = argv[2];

	if ((outPath.length()) && (!outPath.endsWith("/")))
	{
		outPath.append("/");
	}
	QStringList cmdList;
	qDebug()<<"Input: " << path;
	//check if we are in a card's root
	if (path.endsWith("CONTENTS") || QDir(path).entryList().contains("CONTENTS"))
	{
		cmdList = convertFolderCmds(QDir::currentPath(), outPath);
	}
	else //try one level deeper
	{
		QDir dir(path);
		QFileInfoList dirs = dir.entryInfoList();
		foreach(QFileInfo f, dirs)
		{
			if (f.isDir())
			{
				cmdList.append(convertFolderCmds(f.absoluteFilePath(), outPath));
			}
		}
	}
	foreach(QString cmd, cmdList)
	{
		qDebug()<<cmd;
		QProcess process;
		process.start("ffmpeg", cmd.split(" "	));
		process.waitForFinished(-1);
	}


	//return a.exec();
	return 0;
}
