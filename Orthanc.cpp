#include "Orthanc.h"
#include "Api.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QtConcurrent/QtConcurrent>

static const QString DICOM_DATE_FORMAT = "yyyyMMdd";
static const QString DICOM_TIME_FORMAT = "HHmmss";
QString OrthancInternal::m_host = "127.0.0.1";

static const QMap<Orthanc::ResourceType, QString> RESOURCE_NAME {
    {Orthanc::Patient, "patients"},
    {Orthanc::Study, "studies"},
    {Orthanc::Series, "series"},
    {Orthanc::Instance, "instances"}
};

static const QMap<Orthanc::PatientSex, QString> SEX_NAME {
    {Orthanc::Male, "M"},
    {Orthanc::Female, "F"},
    {Orthanc::Other, "O"},
    {Orthanc::None, ""}
};

OrthancInternal::OrthancInternal(QObject *parent, QString address):
    QObject(parent),
    m_api(new Api(address, 8042, this))
{
}

void OrthancInternal::setHost(const QString &host)
{
    m_host = host;
}

void Orthanc::setHost(const QString &host)
{
    OrthancInternal::setHost(host);
}

QString OrthancInternal::host()
{
    return m_host;
}

QString Orthanc::host()
{
    return OrthancInternal::host();
}

Orthanc::EchoOutcome OrthancInternal::echo(const QString &modality)
{
    Api::CallResult result = m_api->call(QString("modalities/%1/echo").arg(modality),
                                         Api::POST, QByteArray());
    return Orthanc::EchoOutcome(result.serverOk, result.resourceFound);
}

bool OrthancInternal::remove(Orthanc::ResourceType type, const QString &id)
{
    Api::CallResult result = m_api->call(QString("%1/%2").arg(RESOURCE_NAME[type]).arg(id),
                                         Api::DELETE, QByteArray());
    return result.serverOk && result.resourceFound;
}

void OrthancInternal::list(Orthanc::ResourceType type)
{
    //TODO finish this method
    m_api->call(RESOURCE_NAME[type], Api::GET, QByteArray());
}

Orthanc::ModalitiesOutcome OrthancInternal::listModalities()
{
    Api::CallResult result = m_api->call("modalities", Api::GET, QByteArray());
    if (result.serverOk) {
        QStringList modalities;
        if (result.resourceFound) {
            QJsonArray arr = QJsonDocument::fromJson(result.answer).array();
            for (auto value: arr) {
                modalities.append(value.toString());
            }
        }
        return Orthanc::ModalitiesOutcome(true, modalities);
    } else {
        return Orthanc::ModalitiesOutcome(false);
    }
}

QString OrthancInternal::publicId(Orthanc::ResourceType type, const QString &id)
{
    Api::CallResult result = m_api->call(QString("public-id/%1/%2").arg(RESOURCE_NAME[type]).arg(id),
                                         Api::GET, QByteArray());
    if (result.serverOk && result.resourceFound) {
        QJsonDocument doc = QJsonDocument::fromJson(result.answer);
        return doc.array().at(0).toString();
    }
    return QString();
}

Orthanc::DicomDirOutcome OrthancInternal::dicomdir(Orthanc::ResourceType type, const QString &id)
{
    Api::CallResult result = m_api->call(QString("%1/%2/media").arg(RESOURCE_NAME[type]).arg(publicId(type,id)),
                                         Api::GET, QByteArray());
    bool success = false;
    QByteArray raw;
    if (result.serverOk) {
        success = true;
        if (result.resourceFound) {
            raw = result.answer;
        }
    }
    return Orthanc::DicomDirOutcome(success, raw);
}

Orthanc::WorklistOutcome OrthancInternal::worklist(const QString &modality, const Orthanc::WorklistRequest &request)
{
    QJsonObject wlRequestJson;
    if (!request.patientName.isEmpty()) {
        wlRequestJson["PatientName"] = QString("*%1*").arg(QString(request.patientName).
                                                           replace(" ", "?"));
    } else {
        wlRequestJson["PatientName"] = "";
    }
    wlRequestJson["PatientSex"] = SEX_NAME[request.sex];

    QJsonObject stepSequence;
    stepSequence["Modality"] = request.modality;

    QString stepStartDate;
    if (!request.startDateFrom.isNull() || !request.startDateTo.isNull()) {
        stepStartDate = QString("%1-%2").
                arg(request.startDateFrom.toString(DICOM_DATE_FORMAT)).
                arg(request.startDateTo.toString(DICOM_DATE_FORMAT));
    }
    stepSequence["ScheduledProcedureStepStartDate"] = stepStartDate;

    QJsonArray stepSequenceArray;
    stepSequenceArray.append(stepSequence);

    wlRequestJson["ScheduledProcedureStepSequence"] = stepSequenceArray;
    Api::CallResult result = m_api->call(QString("modalities/%1/sps-find").arg(modality),
                                         Api::POST, QJsonDocument(wlRequestJson).toJson());
    bool success  = false;
    Orthanc::Worklist wl;
    if (result.serverOk) {
        success = true;
        if (result.resourceFound) {
            QJsonArray array = QJsonDocument::fromJson(result.answer).array();
            for (auto jsonItem: array) {
                if (jsonItem.isObject()) {
                    QJsonObject obj = jsonItem.toObject();
                    wl.append({
                        obj["PatientName"].toString().replace("^", " "),
                        obj["PatientName"].toString(),
                        QDate::fromString(obj["PatientBirthDate"].toString(), DICOM_DATE_FORMAT),
                        SEX_NAME.key(obj["PatientSex"].toString()),
                        obj["Modality"].toString(),
                        QDate::fromString(obj["StartDate"].toString(), DICOM_DATE_FORMAT),
                        QTime::fromString(obj["StartTime"].toString(), DICOM_TIME_FORMAT),
                        obj["Description"].toString(),
                        obj["UUID"].toString(),
                        obj["PatientID"].toString(),
                        obj["StudyInstanceUID"].toString(),
                        obj["AccessionNumber"].toString()
                    });
                }
            }
        }
    }
    return Orthanc::WorklistOutcome(success, wl);
}

Orthanc::StoreOutcome OrthancInternal::store(Orthanc::ResourceType type, const QString &id, const QString &modality)
{
    Api::CallResult result = m_api->call(QString("/modalities/%1/store").arg(modality),
                                         Api::POST, publicId(type, id).toLatin1());
    return Orthanc::StoreOutcome(result.serverOk, result.resourceFound);
}

Orthanc::PrintOutcome OrthancInternal::print(const QString &modality, const Orthanc::PrintSettings &settings)
{
    QJsonObject request;
    request["NumberOfCopies"] = QJsonValue(settings.nCopies);
    request["PrintPriority"] = settings.priority;
    request["MediumType"] = settings.medium;
    request["FilmDestination"] = settings.filmDestination;
    request["FilmOrientation"] = settings.filmOrientation;
    request["FilmSizeID"] = settings.filmSizeId;
    request["MagnificationType"] = settings.magnification;
    request["SmoothingType"] = settings.smoothing;
    request["Trim"] = settings.trim;

    QByteArray arr;
    QBuffer pngBuff(&arr);
    pngBuff.open(QIODevice::WriteOnly);
    settings.img.save(&pngBuff, "PNG");
    request["Image"] = QString("data:image/png;base64,%1").arg(QString(arr.toBase64()));

    Api::CallResult result = m_api->call(QString("modalities/%1/print").arg(modality),
                                         Api::POST, QJsonDocument(request).toJson());

    return Orthanc::PrintOutcome(result.serverOk, result.resourceFound);
}

Orthanc::SpsOutcome OrthancInternal::spsStart(const QString &modality, const QString &uuid)
{
    Api::CallResult result = m_api->call(QString("modalities/%1/sps-start/%2").
                                            arg(modality).arg(uuid),
                                         Api::POST, QByteArray());
    return Orthanc::SpsOutcome(result.serverOk, result.resourceFound);
}

Orthanc::SpsOutcome OrthancInternal::spsCancel(const QString &modality)
{
    Api::CallResult result = m_api->call(QString("modalities/%1/sps-cancel").arg(modality),
                                         Api::POST, QByteArray());
    return Orthanc::SpsOutcome(result.serverOk, result.resourceFound);
}

Orthanc::SpsOutcome OrthancInternal::spsComplete(const QString &modality, const Orthanc::PerformedProcedureSteps &steps)
{
    QJsonArray stepsArray;
    for (auto step: steps) {
        QJsonObject stepObject;
        stepObject["OperatorsName"] = step.operatorsName;
        stepObject["SeriesInstanceUID"] = step.seriesInstanceUID;
        stepObject["RetrieveAETitle"] = step.retrieveAETitle;
        stepsArray.append(stepObject);
    }
    Api::CallResult result = m_api->call(QString("modalities/%1/sps-complete").arg(modality),
                                         Api::POST, QJsonDocument(stepsArray).toJson());
    return Orthanc::SpsOutcome(result.serverOk, result.resourceFound);
}

Orthanc::AeTitleOutcome OrthancInternal::getAeTitle(const QString &modality)
{
    Api::CallResult result = m_api->call(QString("modalities?expand"), Api::GET, QByteArray());
    QString aeTitle;
    if (result.serverOk && result.resourceFound) {
        QJsonObject json = QJsonDocument::fromJson(result.answer).object();
        if (json.contains(modality)) {
            QJsonArray modalityInfo = json[modality].toArray();
            aeTitle = modalityInfo[0].toString();
        }
    }
    return Orthanc::AeTitleOutcome(result.serverOk && result.resourceFound,
                                   aeTitle);
}

Orthanc::ReloadOutcome OrthancInternal::reload()
{
    Api::CallResult result = m_api->call(QString("tools/reload"), Api::POST, QByteArray());
    return Orthanc::ReloadOutcome(result.serverOk, result.resourceFound);
}

void OrthancInternal::restart()
{
    m_api->call("tools/reset", Api::POST, QByteArray());
}

Orthanc::Orthanc(QObject *parent, QString address) :
    QObject(parent),
    m_orthanc(new OrthancInternal(this, address))
{
}

QFuture<Orthanc::EchoOutcome> Orthanc::echo(const QString &modality)
{
    return QtConcurrent::run(m_orthanc, &OrthancInternal::echo, modality);
}

QFuture<Orthanc::ModalitiesOutcome > Orthanc::listModalities()
{
    return QtConcurrent::run(m_orthanc, &OrthancInternal::listModalities);
}

QFuture<Orthanc::DicomDirOutcome> Orthanc::dicomdir(Orthanc::ResourceType type, const QString &id)
{
    return QtConcurrent::run(m_orthanc, &OrthancInternal::dicomdir, type, id);
}

QFuture<Orthanc::WorklistOutcome> Orthanc::worklist(const QString &modality, const WorklistRequest &request)
{
    return QtConcurrent::run(m_orthanc, &OrthancInternal::worklist, modality, request);
}

QFuture<Orthanc::StoreOutcome> Orthanc::store(Orthanc::ResourceType type, const QString &id, const QString &modality)
{
    return QtConcurrent::run(m_orthanc, &OrthancInternal::store, type, id, modality);
}

QFuture<Orthanc::PrintOutcome> Orthanc::print(const QString &modality, const PrintSettings &settings)
{
    return QtConcurrent::run(m_orthanc, &OrthancInternal::print, modality, settings);
}

QFuture<Orthanc::SpsOutcome> Orthanc::spsStart(const QString &modality, const QString &uuid)
{
    return QtConcurrent::run(m_orthanc, &OrthancInternal::spsStart, modality, uuid);
}

QFuture<Orthanc::SpsOutcome> Orthanc::spsCancel(const QString &modality)
{
    return QtConcurrent::run(m_orthanc, &OrthancInternal::spsCancel, modality);
}

QFuture<Orthanc::SpsOutcome> Orthanc::spsComplete(const QString &modality, const PerformedProcedureSteps &steps)
{
    return QtConcurrent::run(m_orthanc, &OrthancInternal::spsComplete, modality, steps);
}

QFuture<Orthanc::AeTitleOutcome> Orthanc::getAeTitle(const QString &modality)
{
    return QtConcurrent::run(m_orthanc, &OrthancInternal::getAeTitle, modality);
}

QFuture<Orthanc::ReloadOutcome> Orthanc::reload()
{
    return QtConcurrent::run(m_orthanc, &OrthancInternal::reload);
}
