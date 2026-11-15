# Third-party licensing — working notes

## JUCE

The current plugin foundation fetches JUCE 9.0.5 from the official JUCE repository.

JUCE's framework modules are dual-licensed under the AGPLv3 and the commercial JUCE licence. A closed-source commercial plugin must not assume that the AGPL route is compatible with its distribution model. Select and maintain an appropriate JUCE commercial tier, or obtain qualified legal advice before distributing closed-source binaries.

The official JUCE pricing page currently lists:
- Starter: free, revenue up to USD 20,000.
- Indie: revenue up to USD 300,000.
- Pro: unlimited revenue.
- Educational: subject to educational eligibility.

These limits and terms can change. Verify the current JUCE EULA and account eligibility directly before release. No JUCE licence has been purchased or selected as part of this repository work.

Official references:
- https://github.com/juce-framework/JUCE/blob/master/LICENSE.md
- https://juce.com/get-juce/
- https://juce.com/legal/juce-9-licence/

## Other dependencies

JUCE's repository includes a software bill of materials for third-party components. Before release, audit the exact components enabled by this project and retain the notices required by their licences.

## Release gate

Do not distribute a closed-source commercial build until:
1. The correct JUCE licence tier is confirmed and maintained.
2. Enabled JUCE components and transitive dependencies are reviewed.
3. VST3 SDK and any other bundled dependency notices are reviewed.
4. The product's own licence and third-party notices are prepared.
