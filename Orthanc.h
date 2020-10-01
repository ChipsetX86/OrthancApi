#ifndef ORTHANC_H
#define ORTHANC_H

#include "CommonGlobal.h"

#include <QObject>
#include <QJsonDocument>
#include <QFuture>
#include <QDateTime>
#include <QImage>

class Api;
class OrthancInternal;

class COMMON_LIBRARY_EXPORT Orthanc : public QObject
{
    Q_OBJECT
public:
    template <typename R>
    class Outcome
    {
        friend class OrthancInternal;
    public:
        explicit Outcome()
        {
            Outcome(false);
        }
        inline R result() const {return m_result;}
        bool success() const {return m_success;}
    private:
        Outcome(bool success, R result = R()):
            m_result(result),
            m_success(success)
        {
        }
        R m_result;
        bool m_success;
    };

    enum ResourceType {
        Patient,
        Study,
        Series,
        Instance
    };
    enum PatientSex
    {
        Male,
        Female,
        Other,
        None
    };
    struct WorklistRequest
    {
        QString patientName;
        PatientSex sex = None;
        QString modality;
        QDate startDateFrom;
        QDate startDateTo;
    };
    struct WorklistItem
    {
        QString patientName;
        QString dicomPatientName;
        QDate patientBirthDate;
        PatientSex sex;
        QString modality;
        QDate startDate;
        QTime startTime;
        QString description;
        QString uuid;
        QString patientId;
        QString studyInstanceUID;
        QString accessionNumber;
    };
    using Worklist = QList<WorklistItem>;

    struct PrintSettings {
        int nCopies;
        QString priority;
        QString medium;
        QString filmDestination;
        QString filmOrientation;
        QString filmSizeId;
        QString magnification;
        QString smoothing;
        QString trim;
        QImage img;
    };

    struct PerformedProcedureStep {
        QString operatorsName;
        QString seriesInstanceUID;
        QString retrieveAETitle;
    };
    using PerformedProcedureSteps = QVector<PerformedProcedureStep>;

    using EchoOutcome = Outcome<bool>;
    using ModalitiesOutcome = Outcome<QStringList>;
    using DicomDirOutcome = Outcome<QByteArray>;
    using WorklistOutcome = Outcome<Worklist>;
    using StoreOutcome = Outcome<bool>;
    using PrintOutcome = Outcome<bool>;
    using SpsOutcome = Outcome<bool>;
    using AeTitleOutcome = Outcome<QString>;
    using ReloadOutcome = Outcome<bool>;

    explicit Orthanc(QObject *parent = nullptr, QString address = Orthanc::host());
    QFuture<EchoOutcome> echo(const QString &modality);
    QFuture<ModalitiesOutcome> listModalities();
    QFuture<DicomDirOutcome> dicomdir(Orthanc::ResourceType type, const QString &id);
    QFuture<WorklistOutcome> worklist(const QString &modality, const WorklistRequest &request);
    QFuture<StoreOutcome> store(Orthanc::ResourceType type, const QString &id, const QString &modality);
    QFuture<PrintOutcome> print(const QString &modality, const PrintSettings &settings);
    QFuture<SpsOutcome> spsStart(const QString &modality, const QString &uuid);
    QFuture<SpsOutcome> spsCancel(const QString &modality);
    QFuture<SpsOutcome> spsComplete(const QString &modality, const PerformedProcedureSteps& steps);
    QFuture<AeTitleOutcome> getAeTitle(const QString &modality);
    QFuture<ReloadOutcome> reload();
    static void setHost(const QString &host);
    static QString host();
private:
    OrthancInternal *m_orthanc;

};

class OrthancInternal: public QObject {
    Q_OBJECT
public:
    explicit OrthancInternal(QObject *parent = nullptr, QString address = OrthancInternal::host());
    Orthanc::EchoOutcome echo(const QString &modality);
    bool remove(Orthanc::ResourceType type, const QString &id);
    void list(Orthanc::ResourceType type);
    Orthanc::DicomDirOutcome dicomdir(Orthanc::ResourceType type, const QString &id);
    Orthanc::WorklistOutcome worklist(const QString &modality, const Orthanc::WorklistRequest &request);
    Orthanc::StoreOutcome store(Orthanc::ResourceType type, const QString &id, const QString &modality);
    Orthanc::PrintOutcome print(const QString &modality, const Orthanc::PrintSettings &settings);
    Orthanc::ModalitiesOutcome listModalities();
    Orthanc::SpsOutcome spsStart(const QString &modality, const QString &uuid);
    Orthanc::SpsOutcome spsCancel(const QString &modality);
    Orthanc::SpsOutcome spsComplete(const QString &modality, const Orthanc::PerformedProcedureSteps &steps);
    Orthanc::AeTitleOutcome getAeTitle(const QString &modality);
    Orthanc::ReloadOutcome reload();
    void restart();
    static void setHost(const QString &host);
    static QString host();
private:
    QString publicId(Orthanc::ResourceType type, const QString &id);
    Api *m_api;
    static QString m_host;
};

#endif // ORTHANC_H
