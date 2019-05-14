# Bluetooth PBAP Service

## Overview

Bluetooth PBAP service reports respective vCard phonebook data from BlueZ via communication with PBAP profile

## Verbs

| Name        | Description                               | JSON Response                                      |
|-------------|-------------------------------------------|----------------------------------------------------|
| subscribe   | subscribe to Bluetooth PBAP events        | *Request:* {"value": "status"}                     |
| unsubscribe | unsubscribe to Bluetooth PBAP events      | *Request:* {"value": "status"}                     |
| contacts    | return all contacts from connected device | see **contacts verb section**                      |
| entry       | return vCard data from handle             | see **entry verb section**                         |
| history     | return call history list                  | see **history verb section**                       |
| search      | search for respective vCard handle        | see **search verb section**                        |
| status      | current device connection status          | same response as noted in **status event section** |

### contacts Verb

Returns all vCards that are accessible from respective connected device in concatenated output:

<pre>

"response": {
    "vcards": [
     {
        "fn": "Art McGee",
        "photo": {
            "mimetype": "image/jpeg",
            "data": 'BASE64 IMAGE BLOB HERE'
        },
        "telephone": [
          {
            "CELL": "+13305551212"
          }
        ]
     },
     ...
  ]
}
</pre>

### search Verb

Example of a request for vCard search using **number** parameter (i.e. *{"number":"+15035551212"}*) results:

<pre>
"response": {
    "results": [
       {
          "handle": "27e.vcf",
          "name": "Art McGee"
       }
    ]
}                                       },
</pre>

### entry Verb

Client must pass one of the following values to the **list** parameter in request:

| Value         | Description                                      |
|---------------|--------------------------------------------------|
| ich           | Incoming calls                                   |
| och           | Outgoing calls                                   |
| mch           | Missed calls                                     |
| cch           | Combined calls (e.g. incoming, outgoing, missed) |
| pb            | Phonebook (typically selected)                   |

Also there is a **handle** parameter that must be in form of vCard path (e.g. 27e.vcf).

Response is the same as noted in the **contacts** verb

### history Verb

Client must pass one of the following values to the list parameter in request:

| Value         | Description                                      |
|---------------|--------------------------------------------------|
| ich           | Incoming calls                                   |
| och           | Outgoing calls                                   |
| mch           | Missed calls                                     |
| cch           | Combined calls (e.g. incoming, outgoing, missed) |

Sample request for a combined list (i.e. *{"list":"cch"}*) and its respective response:

<pre>
"response": {
    "vcards": [
      {
        "fn": "Art McGee"
        "type": "DIALED",
        "timestamp": "20190509T193422",
        "telephone": "+13305551212"
      },
      {
        "fn": "UNKNOWN CALLER",
        "type": "MISSED",
        "timestamp": "20190426T014109",
        "telephone": "+15035551212"
      },
      {
        "fn": "Satoshi Nakamoto"
        "type": "RECEIVED",
        "timestamp": "20190421T090123",
        "telephone": "+13605551212"
      }
   ]
}
</pre>

## Events

Respective binding only outputs one event that signals if an PBAP capable device is connected

### status Event

Sample of a Bluetooth PBAP status event:

<pre>
{
  "connected": true
}
</pre>
