/*
 * Copyright (c) 2026 FutureNet Team
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * M10: ActionValidator interface and implementation.
 */

#ifndef FUTURENET_ACTION_VALIDATOR_H
#define FUTURENET_ACTION_VALIDATOR_H

#include "action-handler.h"
#include "futurenet-agent-types.h"
#include "ns3/boolean.h"
#include "ns3/object.h"
#include <vector>

namespace ns3 {
namespace futurenet {

/**
 * \ingroup futurenet-agent
 * \brief Validates external agent actions before dispatch to ActionHandlers.
 * Defined in SRS Section 5 (M10-FR-05) and Section 8.
 */
class ActionValidator : public Object
{
public:
    static TypeId GetTypeId();

    ActionValidator();
    ~ActionValidator() override;

    /**
     * \brief Validate an incoming action against the current observation context and registered handlers.
     * \param action The proposed action to validate.
     * \param currentObs The active observation snapshot.
     * \param handlers List of currently registered ActionHandlers.
     * \return ValidationResult indicating whether action is accepted or rejected with explanation.
     */
    ValidationResult Validate(const AgentAction& action,
                             const AgentObservation& currentObs,
                             const std::vector<Ptr<ActionHandler>>& handlers) const;

    /**
     * \brief Enable or disable strict decision ID checks.
     * \param strict If true, action decision ID must match observation decision ID exactly.
     */
    void SetStrictValidation(bool strict);

    /**
     * \brief Check whether strict validation is enabled.
     * \return true if strict validation is active.
     */
    bool GetStrictValidation() const;

private:
    bool m_strictValidation{true}; //!< Attribute: StrictActionValidation
};

} // namespace futurenet
} // namespace ns3

#endif // FUTURENET_ACTION_VALIDATOR_H
