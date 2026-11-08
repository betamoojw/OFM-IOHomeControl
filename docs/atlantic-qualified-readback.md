# Qualified Atlantic readback jobs

The existing bounded register-job engine now hosts typed Adelis and Calypso actions. Raw read jobs still cannot encode writes. State-changing stages are fixed by the typed operation and validate the model, stored peer key, system key, gateway, channel revision, token, frequency and deadlines. Each stage uses the real controller queue, frame builder and challenge-response engine. A distinct read stage follows a write; there is no generic 0C61 console or blind write retry.

Adelis qualification starts with an authenticated 0119 model-name read in that job. Calypso target admission reads both 200-litre volume and manual mode before sending. Final observations never borrow unsolicited inbound trust or position/rain/error publication. The native workflow tests exercise real routing, payloads, challenge handling, wrong peer/frequency/key/system-key/revision/model, timeout, cancellation, wrong name, AUTO and another tank size. Community captures supply functional replies; all test authentication material is explicitly synthetic.

Header/model authorization is a runtime gate, not physical effect evidence. Readback agreement means the observed register/status matches the requested value; it does not prove causation, motion or heat delivery. Physical mode/Boost/Drying/target acceptance and the hardware gates in the source plan remain separate.
