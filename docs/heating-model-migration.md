# Explicit heating model selection

Source: [Adelis and AGILIA share the same protocol type](https://github.com/laberning/home_io_control/commit/3923fd5f9a4f54e785186f28f0c43ada7b4dd7c7). OFM module version is 0.10.0; the matching OAM application advances from 4.9 to 4.10 and verifies module 0.10.

The HTG union remains three bytes. A new two-bit model choice occupies previously reserved bits at offset 0/bit offset 3: Unknown=0, AGILIA=1, Adelis=2. Every existing parameter ID, offset, default and KO address/DPT remains unchanged. Configuration transfer cannot copy the model authorization. The new getter fails closed without the generated parameter, for invalid enum values and when HTG is disabled.

Existing projects retain their previous settings, but their previously unused model bits are zero: an operator must explicitly choose the physical model in the new application. No old 52/1 type automatically authorizes either model. AGILIA objects are displayed only for AGILIA; Adelis exposes no AGILIA window/contact/detection objects. Adelis controls initially use the local typed diagnostic interface, with a live name read before a state-changing job. No new measurement KOs or altered DPT interpretation are introduced.

Changing the selected model invalidates the runtime product/heating context. Node, key, gateway, system-key and revision checks invalidate qualified names and jobs independently. ETS choice is a configuration declaration; a matching live name and transaction binding supply runtime qualification. No physical name match or firmware build substitutes for testing the model on its actual device.

`test_heating_model_schema.py` checks reserved-bit use, unchanged old parameters/KOs and the actual production getter. Producer 4.3.12 expands the new XML and generates the header; existing macros differ only in application/module versions, plus the four new model macros. Signing/packing a final knxprod still requires an ETS installation; this Mac's producer reports that ETS is absent.
