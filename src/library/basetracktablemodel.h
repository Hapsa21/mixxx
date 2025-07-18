#pragma once

#include <QAbstractTableModel>
#include <QList>
#include <QPointer>
#include <optional>

#include "library/columncache.h"
#include "library/trackmodel.h"
#include "track/track_decl.h"
#include "util/color/colorpalette.h"

class TrackCollectionManager;

/// Base class for tabular track list views.
///
/// The abstract model behind `WTrackTableView`.
///
/// Closely coupled with `BaseSqlTableModel` from which it has been extracted once.
///
/// Serves as an extension point for integrating external track data into Mixxx.
/// It allows to view track lists provided by external libraries using `WTrackTableView`
/// without importing redundant data into the Mixxx database.
class BaseTrackTableModel : public QAbstractTableModel, public TrackModel {
    Q_OBJECT
    DISALLOW_COPY_AND_ASSIGN(BaseTrackTableModel);

  public:
    explicit BaseTrackTableModel(
            QObject* parent,
            TrackCollectionManager* const pTrackCollectionManager,
            const char* settingsNamespace);
    ~BaseTrackTableModel() override = default;

    QVariant getFieldVariant(const QModelIndex& index, ColumnCache::Column column) const;
    QVariant getFieldVariant(const QModelIndex& index, const QString& fieldName) const;
    QString getFieldString(const QModelIndex& index, ColumnCache::Column column) const;

    ///////////////////////////////////////////////////////
    //  Overridable functions
    ///////////////////////////////////////////////////////

    virtual int fieldIndex(
            ColumnCache::Column column) const {
        return m_columnCache.fieldIndex(column);
    }

    virtual int endFieldIndex() const {
        return m_columnCache.endFieldIndex();
    }

    ///////////////////////////////////////////////////////
    // Inherited from QAbstractItemModel
    ///////////////////////////////////////////////////////

    QVariant headerData(
            int section,
            Qt::Orientation orientation,
            int role = Qt::DisplayRole) const final;
    bool setHeaderData(
            int section,
            Qt::Orientation orientation,
            const QVariant& value,
            int role = Qt::DisplayRole) final;

    QMimeData* mimeData(
            const QModelIndexList& indexes) const final;

    QVariant data(
            const QModelIndex& index,
            int role = Qt::DisplayRole) const final;
    bool setData(
            const QModelIndex& index,
            const QVariant& value,
            int role = Qt::EditRole) final;

    // Calculate the number of columns from all valid
    // column headers.
    // Reimplement in derived classes if a more efficient
    // implementation is available.
    int columnCount(
            const QModelIndex& parent = QModelIndex()) const override;

    // Calls readWriteFlags() by default
    // Reimplement in derived classes if the table model
    // should be readOnly
    Qt::ItemFlags flags(
            const QModelIndex& index) const override;

    ///////////////////////////////////////////////////////
    // Inherited from TrackModel
    ///////////////////////////////////////////////////////

    QAbstractItemDelegate* delegateForColumn(
            const int column,
            QObject* pParent) final;

    int fieldIndex(
            const QString& fieldName) const override {
        return m_columnCache.fieldIndex(fieldName);
    }

    void cutTracks(const QModelIndexList& indices) override;
    void copyTracks(const QModelIndexList& indices) const override;
    QList<int> pasteTracks(const QModelIndex& index) override;

    bool isColumnHiddenByDefault(
            int column) override;

    TrackPointer getTrackByRef(
            const TrackRef& trackRef) const override;

    bool updateTrackGenre(
            Track* pTrack,
            const QString& genre) const override;
#if defined(__EXTRA_METADATA__)
    bool updateTrackMood(
            Track* pTrack,
            const QString& mood) const override;
#endif // __EXTRA_METADATA__

    static constexpr int kBpmColumnPrecisionDefault = 1;
    static constexpr int kBpmColumnPrecisionMinimum = 0;
    static constexpr int kBpmColumnPrecisionMaximum = 10;
    static void setBpmColumnPrecision(int precision);

    static constexpr bool kKeyColorsEnabledDefault = true;
    static void setKeyColorsEnabled(bool keyColorsEnabled);

    static void setKeyColorPalette(const ColorPalette& palette);

    static constexpr bool kApplyPlayedTrackColorDefault = true;
    static void setApplyPlayedTrackColor(bool apply);

  protected:
    // Build a map from the column names to their indices
    // used by fieldIndex().
    void initTableColumnsAndHeaderProperties(
            const QStringList& tableColumns);

    QString columnNameForFieldIndex(int index) const {
        return m_columnCache.columnNameForFieldIndex(index);
    }

    void setHeaderProperties(ColumnCache::Column column);

    ColumnCache::Column mapColumn(int column) const {
        if (column >= 0 && column < m_columnHeaders.size()) {
            return m_columnHeaders[column].column;
        } else {
            return ColumnCache::COLUMN_LIBRARYTABLE_INVALID;
        }
    }

    // Emit the dataChanged() signal for multiple rows in
    // a single column. The list of rows must be sorted in
    // ascending order without duplicates!
    void emitDataChangedForMultipleRowsInColumn(
            const QList<int>& rows,
            int column,
            const QVector<int>& roles = QVector<int>());

    const TrackId previewDeckTrackId() const {
        return m_previewDeckTrackId;
    }

    bool isBpmLocked(
            const QModelIndex& index) const;

    const QPointer<TrackCollectionManager> m_pTrackCollectionManager;

    ///////////////////////////////////////////////////////
    //  Overridable functions
    ///////////////////////////////////////////////////////

    // Use this if you want a model that is read-only.
    virtual Qt::ItemFlags readOnlyFlags(
            const QModelIndex& index) const;
    // Use this if you want a model that can be changed
    virtual Qt::ItemFlags readWriteFlags(
            const QModelIndex& index) const;

    /// Return the raw data value at the given index.
    ///
    /// Expected types by ColumnCache field (pass-through = not validated):
    /// COLUMN_LIBRARYTABLE_ID: int (pass-through)
    /// COLUMN_LIBRARYTABLE_ARTIST: QString (pass-through)
    /// COLUMN_LIBRARYTABLE_TITLE: QString (pass-through)
    /// COLUMN_LIBRARYTABLE_ALBUM: QString (pass-through)
    /// COLUMN_LIBRARYTABLE_ALBUMARTIST: QString (pass-through)
    /// COLUMN_LIBRARYTABLE_YEAR: QString (pass-through)
    /// COLUMN_LIBRARYTABLE_GENRE: QString (pass-through)
    /// COLUMN_LIBRARYTABLE_COMPOSER: QString (pass-through)
    /// COLUMN_LIBRARYTABLE_GROUPING: QString (pass-through)
    /// COLUMN_LIBRARYTABLE_TRACKNUMBER: QString (pass-through)
    /// COLUMN_LIBRARYTABLE_FILETYPE: QString (pass-through)
    /// COLUMN_TRACKLOCATIONSTABLE_LOCATION: QString (pass-through)
    /// COLUMN_LIBRARYTABLE_COMMENT: QString (pass-through)
    /// COLUMN_LIBRARYTABLE_DURATION: double (seconds)/mixxx::Duration
    /// COLUMN_LIBRARYTABLE_BITRATE: int (kbps)/mixxx::audio::Bitrate
    /// COLUMN_LIBRARYTABLE_BPM: double (beats per minute)/mixxx::Bpm
    /// COLUMN_LIBRARYTABLE_REPLAYGAIN: double (ratio)/mixxx::ReplayGain
    /// COLUMN_LIBRARYTABLE_URL: QString (unsupported)
    /// COLUMN_LIBRARYTABLE_SAMPLERATE: int (Hz)/mixxx::audio::SampleRate (unsupported)
    /// COLUMN_LIBRARYTABLE_WAVESUMMARYHEX: QVariant (pass-through)
    /// COLUMN_LIBRARYTABLE_CHANNELS: int/mixxx::audio::ChannelCount (unsupported)
    /// COLUMN_LIBRARYTABLE_MIXXXDELETED: bool (pass-through)
    /// COLUMN_LIBRARYTABLE_DATETIMEADDED: QDateTime
    /// COLUMN_LIBRARYTABLE_HEADERPARSED: bool (pass-through)
    /// COLUMN_LIBRARYTABLE_TIMESPLAYED: int
    /// COLUMN_LIBRARYTABLE_PLAYED: bool
    /// COLUMN_LIBRARYTABLE_RATING: int
    /// COLUMN_LIBRARYTABLE_KEY: QString (literal key name, pass-through)
    /// COLUMN_LIBRARYTABLE_KEY_ID: int (internal key code)
    /// COLUMN_LIBRARYTABLE_BPM_LOCK: bool
    /// COLUMN_LIBRARYTABLE_PREVIEW: bool
    /// COLUMN_LIBRARYTABLE_COLOR: mixxx::RgbColor::code_t
    /// COLUMN_LIBRARYTABLE_COVERART: virtual column for CoverArtDelegate
    /// COLUMN_LIBRARYTABLE_COVERART_SOURCE: int (pass-through)
    /// COLUMN_LIBRARYTABLE_COVERART_TYPE: int (pass-through)
    /// COLUMN_LIBRARYTABLE_COVERART_LOCATION: QString (pass-through)
    /// COLUMN_LIBRARYTABLE_COVERART_COLOR: mixxx::RgbColor::code_t (pass-through)
    /// COLUMN_LIBRARYTABLE_COVERART_DIGEST: QByteArray (pass-through)
    /// COLUMN_LIBRARYTABLE_COVERART_HASH: quint16 (pass-through)
    /// COLUMN_LIBRARYTABLE_LAST_PLAYED_AT: QDateTime
    /// COLUMN_PLAYLISTTABLE_DATETIMEADDED: QDateTime
    virtual QVariant rawValue(
            const QModelIndex& index) const = 0;

    QVariant roleValue(
            const QModelIndex& index,
            QVariant&& rawValue,
            int role) const;

    virtual bool setTrackValueForColumn(
            const TrackPointer& pTrack,
            int column,
            const QVariant& value,
            int role) {
        Q_UNUSED(pTrack);
        Q_UNUSED(column);
        Q_UNUSED(value);
        Q_UNUSED(role);
        return false;
    }

  private slots:
    void slotTrackChanged(
            const QString& group,
            TrackPointer pNewTrack,
            TrackPointer pOldTrack);

    void slotRefreshCoverRows(
            const QList<int>& rows);

    void slotRefreshOverviewRows(const QList<int>& rows);

    void slotRefreshAllRows();

    void slotCoverFound(
            const QObject* pRequester,
            const CoverInfo& coverInfo,
            const QPixmap& pixmap);

  private:
    QVariant rawSiblingValue(
            const QModelIndex& index,
            ColumnCache::Column siblingField) const;

    // Track models may reference tracks by an external id
    // TODO: TrackId should only be used for tracks from
    // the internal database.
    virtual TrackId doGetTrackId(
            const TrackPointer& pTrack) const;

    QVariant composeCoverArtToolTipHtml(
            const QModelIndex& index) const;

    Qt::ItemFlags defaultItemFlags(
            const QModelIndex& index) const;

    QList<QUrl> collectUrls(
            const QModelIndexList& indexes) const;

    const QString m_previewDeckGroup;

    double m_backgroundColorOpacity;
    QColor m_trackPlayedColor;
    QColor m_trackMissingColor;

    ColumnCache m_columnCache;

    struct ColumnHeader {
        ColumnCache::Column column = ColumnCache::COLUMN_LIBRARYTABLE_INVALID;
        QHash</*role*/ int, QVariant> header;
    };
    QVector<ColumnHeader> m_columnHeaders;

    TrackId m_previewDeckTrackId;

    mutable QModelIndex m_toolTipIndex;

    static int s_bpmColumnPrecision;
    static bool s_keyColorsEnabled;
    // The value need to be left uninitialized (std::nullopt) to avoid static
    // initialization order issues
    static std::optional<ColorPalette> s_keyColorPalette;

    static bool s_bApplyPlayedTrackColor;
};
