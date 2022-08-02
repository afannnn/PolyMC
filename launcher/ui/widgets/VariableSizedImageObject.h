#pragma once

#include <QObject>
#include <QString>
#include <QTextObjectInterface>
#include <QUrl>

/** Custom image text object to be used instead of the normal one in ProjectDescriptionPage.
 *
 *  Why? Because we want to re-scale images dynamically based on the document's size, in order to
 *  not have images being weirdly cropped out in different resolutions.
 */
class VariableSizedImageObject : public QObject, public QTextObjectInterface {
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)

   public:
    QSizeF intrinsicSize(QTextDocument* doc, int posInDocument, const QTextFormat& format) override;
    void drawObject(QPainter* painter, const QRectF& rect, QTextDocument* doc, int posInDocument, const QTextFormat& format) override;

    void setMetaEntry(QString meta_entry) { m_meta_entry = meta_entry; }

   protected:
    /** Adds the image to the document, in the given position.
     */
    void parseImage(QTextDocument* doc, QImage image, int posInDocument);

    /** Loads an image from an external source, and adds it to the document.
     *
     *  This uses m_meta_entry to cache the image.
     */
    void loadImage(QTextDocument* doc, const QUrl& source, int posInDocument);

    QString m_meta_entry;

    QList<QUrl> m_fetching_images;
};
