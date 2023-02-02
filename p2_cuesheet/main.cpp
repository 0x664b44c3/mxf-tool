#include <QCoreApplication>
#include <QFile>
#include <QByteArray>
#include <mxfmeta.h>
#include <QDebug>
#include <QMap>
#include <QDir>
#include <QFileInfo>
#include <iostream>
#include <iomanip>
#include <QImage>
#include <QBuffer>
#include <QCommandLineParser>
#include <QCommandLineOption>
QString cardId(QString cardRoot)
{
	QDir dir(cardRoot);
	if (!dir.entryList().contains("CONTENTS"))
		dir.cd("..");
	cardRoot = dir.absolutePath();
	if (!dir.entryList().contains("CONTENTS"))
		return QString();
	return dir.absolutePath().split("/").last();
}

/** gets a list of all the xml files */
QStringList parseCard(QString cardRoot)
{
    qDebug()<<"parsing data in"<<cardRoot;
	QDir dir(cardRoot);
	QStringList fileList;
	if (!dir.entryList().contains("CONTENTS"))
		dir.cd("..");
	cardRoot = dir.absolutePath();
	if (!dir.entryList().contains("CONTENTS"))
		return QStringList();

	dir.setPath(cardRoot+"/CONTENTS/CLIP");
	dir.setNameFilters(QStringList()<<"*.xml"<<"*.XML");
	//qDebug()<<dir.absolutePath();
	foreach(QFileInfo file, dir.entryInfoList())
	{
		fileList.append(file.absoluteFilePath());
	}
	return fileList;
}

static bool shootStartLessThan(const MXF::ClipInfo &s1, const MXF::ClipInfo &s2)
{
	return s1.metaData().shootStart() < s2.metaData().shootStart();
}


void printClipEntry(const MXF::ClipInfo & clip, QString prefix, int counter, bool html = false, QByteArray thumbData = QByteArray())
{
	int duration = clip.duration() * clip.editUnit().numerator / clip.editUnit().denominator;
	if (html)
	{
		QString clipName = prefix.size()?(prefix + "_"):"";
		clipName += clip.clipName();

		QString entry = "<li>";
		if (thumbData.size())
		{
			entry += QStringLiteral("<img src=\"data:image/png;base64,%1\" alt=\"(thumbnail)\" "
			                        "class=\"thumbnail\" id=\"thumb_%2\" />")
			         .arg(QString(thumbData.toBase64()))
			         .arg(clip.globalClipID());
			entry+="<br />";
		}

		entry+= QStringLiteral("<span class=\"clipname\">%1</span><br/>").arg(clipName);
		entry+= QStringLiteral("<span class=\"duration\">%1</span><br/>")
		        .arg(QString().sprintf("%d:%02d", duration / 60, duration % 60));
		entry+="</li>\n";
		std::cout << entry.toStdString();
	}
	else
	{
		std::cout << "Clip";
		if (counter)
			std::cout << " " << counter;
		std::cout << ": ";
		if (prefix.size())
			std::cout << prefix.toStdString() << "_";
		std::cout << clip.clipName().toStdString()
		          << " ("
		          << duration / 60 << ":" << std::setw(2) << std::setfill('0') << duration % 60
		          << ")\n";
	}
}

int main(int argc, char *argv[])
{	
	QString path = QDir::currentPath();
	if (argc>1)
	{
		path = argv[1];
		QDir sd(path);
		if (!sd.exists())
			return 2;
		path = sd.absolutePath();
		qDebug() << path;
	}

	QStringList xmlList;
	qDebug()<<"Input: " << path;
	//check if we are in a card's root
	if (path.endsWith("CONTENTS") || QDir(path).entryList().contains("CONTENTS"))
	{
        xmlList = parseCard(path);
	}
	else //try one level deeper
	{
		QDir dir(path);
		QFileInfoList dirs = dir.entryInfoList();
		foreach(QFileInfo f, dirs)
		{
			if (f.isDir())
			{
				xmlList.append(parseCard(f.absoluteFilePath()));
			}
		}
	}
	QList<MXF::ClipInfo> clipList;
	QMap<QString, QString> clipSourceMap;

	foreach(QString f, xmlList)
	{
		QFile dataFile(f);
		if (dataFile.open(QFile::ReadOnly))
		{
			QByteArray data = dataFile.readAll();
			MXF::ClipInfo clipData(data);
			clipList.append(clipData);
			QString cardId;
			QStringList path = f.split('/');
			if (path.size()>3)
			{
				path.pop_back();
				path.pop_back();
				path.pop_back();
				cardId = path.last();
			}
			clipSourceMap.insert(clipData.globalClipID(), cardId);

		}
	}

	qSort(clipList.begin(), clipList.end(), shootStartLessThan);

	QMap<QString, MXF::ClipInfo> clipMap;
	foreach(MXF::ClipInfo clip, clipList)
		clipMap.insert(clip.globalClipID(), clip);


	QStringList shotList;

	foreach(MXF::ClipInfo clip, clipList)
	{
		if (clip.relation().connectionTop.isSet())
		{
			if (clip.relation().connectionTop.globalClipId == clip.globalClipID())
			{
				shotList << clip.globalClipID();
			}
		}
		else
		{
			shotList.append(QStringList() << clip.globalClipID());
		}
	}

//    foreach(auto clip, clipList)
//    {
//        QString stc = clip.metaData().shootStart().toString("yyyy-MM-dd_hh:mm:ss");
//        std::cout << "mv " << clipSourceMap[clip.globalClipID()].toStdString() << "_" << clip.clipName().toStdString() << ".avi "
//        << stc.toStdString() << "_"
//                  << clipSourceMap[clip.globalClipID()].toStdString() << "_" << clip.clipName().toStdString()
//                  <<".avi\n";
//    }

//    return 0;
	bool foundIncomplete = false;
	bool html = true;
	QStringList processed;
	//now parse the list of start clips and create lists of filenames
	if (html)
	{
		QFile hdr(":/sitehead.html");
		hdr.open(QIODevice::ReadOnly);
		std::cout << hdr.readAll().constData();
		std::cout << std::endl;
	}
	else
		std::cout << "Clip reference sheet\n";

	foreach(QString startId, shotList)
	{
		MXF::ClipInfo startClip = clipMap[startId];

		if (html)
		{
			std::cout << "<div class=\"shot\">";
			std::cout << "<span class=\"start\">Start time: "
			          << startClip.metaData().shootStart().toString("yyyy-MM-dd HH:mm:ss").toStdString()
			          << "</span>"
			          << "\n";
			std::cout << "<ul>\n";
		}
		else
		{
			std::cout << "Start time: " << startClip.metaData().shootStart().toString("yyyy-MM-dd HH:mm:ss").toStdString()
			          << "\n";
		}

		MXF::ClipInfo clip = startClip;

		int clipCtr = 1;
		while(1)
		{
			QByteArray thumbData;
			if (html) // try reading thumbnail
			{
				QString iconPath = path
				                   + "/"
				                   + clipSourceMap[clip.globalClipID()]
				                   + "/CONTENTS/ICON/";
				iconPath += clip.clipName();
				iconPath += "." + clip.metaData().thumbnail().format;
				QImage icon(iconPath);
				qDebug()<<iconPath;
				qDebug()<<icon.size();
				if (!icon.isNull())
				{
					QBuffer buff(&thumbData);
					icon.save(&buff, "PNG");
				}
			}
			printClipEntry(clip,
			               clipSourceMap[clip.globalClipID()],
			        clipCtr++,
			        html,
			        thumbData);
			processed.push_back(clip.globalClipID());
			if (!clip.relation().connectionNext.isSet())
				break;
			clip = clipMap[clip.relation().connectionNext.globalClipId];
			if (clip.isNull())
			{
				foundIncomplete = true;
				std::cout << "(incomplete)";
				break;
			}
		}
		if (html)
		{
			std::cout << "</ul></div>\n";
		}
		else
			std::cout << "====\n\n";
	}
	//
	if (foundIncomplete)
	{
		if (html)
			std::cout << "<div class=\"warning\">There were incomplete shots</div>\n";
		else
			std::cout << "(!!) There were incomplete shots\n";
	}
	foreach(QString id, processed)
	{
		clipMap.remove(id);
	}
	if (clipMap.size())
	{
		std::cout << "(!!) There are orphaned clips\n";
		foreach (QString id, clipMap.keys())
		{
			MXF::ClipInfo clip = clipMap[id];
			std::cout << "Start time: "
			          << clip.metaData().shootStart().toString("yyyy-MM-dd HH:mm:ss").toStdString()
			          << "\n";
			int duration = clip.duration() * clip.editUnit().numerator / clip.editUnit().denominator;

			std::cout << "Clip: "
			          << clipSourceMap[clip.globalClipID()].toStdString()
			          << "_" << clip.clipName().toStdString()
			          << " ("
			          << duration / 60 << ":" << std::setw(2) << std::setfill('0') << duration % 60
			          << ")\n";
			std::cout << "Relations:\n"
			          << "Top:  "
			          << clipSourceMap[clip.relation().connectionTop.globalClipId].toStdString()
			          << "_"
			          << clip.relation().connectionTop.clipName.toStdString() << "\n";
			if (clip.relation().connectionPrevious.isSet())
				std::cout
				      << "Prev: "
				      << clipSourceMap[clip.relation().connectionPrevious.globalClipId].toStdString()
				      << "_"
				      << clip.relation().connectionPrevious.clipName.toStdString() << "\n";
			if (clip.relation().connectionPrevious.isSet())
				std::cout
				      << "Next: "
				      << clipSourceMap[clip.relation().connectionNext.globalClipId].toStdString()
				      << "_"
				      << clip.relation().connectionNext.clipName.toStdString() << "\n";

		}
	}
	if (html)
	{
		std::cout << "</body></html>\n\n";
	}
	qDebug() << clipList.size() << "clips found";

    std::cout << "\n\n=======\n";


	return 0;
}
