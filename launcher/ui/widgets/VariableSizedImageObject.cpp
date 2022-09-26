#include "VariableSizedImageObject.h"

#include <QAbstractTextDocumentLayout>
#include <QDebug>
#include <QPainter>
#include <QTextObject>

#include "Application.h"

#include "net/NetJob.h"

enum FormatProperties { ImageData = QTextFormat::UserProperty + 1 };

QSizeF VariableSizedImageObject::intrinsicSize(QTextDocument* doc, int posInDocument, const QTextFormat& format)
{
    Q_UNUSED(posInDocument);

    auto image = qvariant_cast<QImage>(format.property(ImageData));
    auto size = image.size();

    auto doc_width = doc->textWidth() - 2 * doc->documentMargin();

    if (size.width() > doc_width)
        size *= doc_width / (double)size.width();

    return { size };
}
void VariableSizedImageObject::drawObject(QPainter* painter,
                                          const QRectF& rect,
                                          QTextDocument* doc,
                                          int posInDocument,
                                          const QTextFormat& format)
{
    if (!format.hasProperty(ImageData)) {
        QUrl image_url{ qvariant_cast<QString>(format.property(QTextFormat::ImageName)) };
        if (m_fetching_images.contains(image_url))
            return;

        loadImage(doc, image_url, posInDocument);
        return;
    }

    auto image = qvariant_cast<QImage>(format.property(ImageData));

    painter->setRenderHint(QPainter::RenderHint::SmoothPixmapTransform);
    painter->drawImage(rect, image);
}

void VariableSizedImageObject::parseImage(QTextDocument* doc, QImage image, int posInDocument)
{
    QTextCursor cursor(doc);
    cursor.setPosition(posInDocument);
    cursor.setKeepPositionOnInsert(true);

    auto image_char_format = cursor.charFormat();

    image_char_format.setObjectType(QTextFormat::ImageObject);
    image_char_format.setProperty(ImageData, image);

    // Qt doesn't allow us to modify the properties of an existing object in the document.
    // So we remove the old one and add the new one with the ImageData property set.
    cursor.deleteChar();
    cursor.insertText(QString(QChar::ObjectReplacementCharacter), image_char_format);
}

void VariableSizedImageObject::loadImage(QTextDocument* doc, const QUrl& source, int posInDocument)
{
    m_fetching_images.append(source);

    MetaEntryPtr entry = APPLICATION->metacache()->resolveEntry(
        m_meta_entry,
        QString("images/%1").arg(QString(QCryptographicHash::hash(source.toEncoded(), QCryptographicHash::Algorithm::Sha1).toHex())));

    auto job = new NetJob(QString("Load Image: %1").arg(source.fileName()), APPLICATION->network());
    job->addNetAction(Net::Download::makeCached(source, entry));

    auto full_entry_path = entry->getFullPath();
    auto source_url = source;
    connect(job, &NetJob::succeeded, [this, doc, full_entry_path, source_url, posInDocument] {
        qDebug() << "Loaded resource at" << full_entry_path;

        QImage image(full_entry_path);
        doc->addResource(QTextDocument::ImageResource, source_url, image);

        parseImage(doc, image, posInDocument);

        // This size hack is needed to prevent the content from being laid out in an area smaller
        // than the total width available (weird).
        auto size = doc->pageSize();
        doc->adjustSize();
        doc->setPageSize(size);

        m_fetching_images.removeOne(source_url);
    });
    connect(job, &NetJob::finished, job, &NetJob::deleteLater);

    job->start();
}
